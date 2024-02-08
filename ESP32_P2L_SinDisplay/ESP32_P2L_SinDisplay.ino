#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>

// Datos de conexión WiFi
const char* ssid = "TU_SSID";
const char* password = "TU_PASSWORD";

// Datos del servidor MQTT
const char* mqttServer = "TU_BROKER";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// Pines para los botones
const int boton1Pin = 0;

Preferences preferences;

// Variables globales para el nuevo número de dispositivo propuesto
long nuevoNumeroDispositivo = 0;
bool esperandoConfirmacion = false;

void setupWiFi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("conectado");
      client.subscribe("p2lD");
      client.subscribe("confirmchange");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentar en 5 segundos");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  if (String(topic) == "p2lD") {
    nuevoNumeroDispositivo = message.toInt();
    esperandoConfirmacion = true;
    Serial.print("Nuevo número de dispositivo propuesto recibido: ");
    Serial.println(nuevoNumeroDispositivo);
  } else if (String(topic) == "confirmchange" && esperandoConfirmacion) {
    if (message == "1") {
      preferences.putLong("dispositivo", nuevoNumeroDispositivo);
      Serial.print("Cambio de número de dispositivo confirmado: ");
      Serial.println(nuevoNumeroDispositivo);
      esperandoConfirmacion = false;
    }
  }
}

void setup() {
  Serial.begin(115200);
  setupWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  pinMode(boton1Pin, INPUT_PULLUP);

  preferences.begin("p2l", false);
  long numeroDispositivo = preferences.getLong("dispositivo", 1); // Por defecto es 1
  Serial.print("Número de dispositivo: ");
  Serial.println(numeroDispositivo);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int currentButtonState = digitalRead(boton1Pin);
  static int lastButtonState = HIGH;

  if (currentButtonState != lastButtonState && currentButtonState == LOW) {
    Serial.println("Botón 1 presionado");
    client.publish("p2linput", String(preferences.getLong("dispositivo", 1)).c_str());
  }
  lastButtonState = currentButtonState;
}
