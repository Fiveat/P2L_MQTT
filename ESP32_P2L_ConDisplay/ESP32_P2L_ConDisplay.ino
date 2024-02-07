#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include "heltec.h" // Asegúrate de haber instalado la biblioteca Heltec para ESP32

// Datos de conexión WiFi
const char* ssid = "MiFibra-112A";
const char* password = "cJhc3ygH";

// Datos del servidor MQTT
const char* mqttServer = "194.30.15.135";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
Preferences preferences;

// Variables globales para el nuevo número de dispositivo propuesto
long nuevoNumeroDispositivo = 0;
bool esperandoConfirmacion = false;

// Configuración del botón
const int botonPin = 0; // Cambiar según el pin del botón conectado

void setupWiFi() {
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Conectando a WiFi");
  Heltec.display->drawString(0, 10, ssid);
  Heltec.display->display();

  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");

  Heltec.display->drawString(0, 20, "Conectado!");
  Heltec.display->drawString(0, 30, "RSSI: " + String(WiFi.RSSI()) + "dBm");
  Heltec.display->display();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("conectado");
      client.subscribe("p2lD");
      client.subscribe("confirmchange");
      client.subscribe("displayvalue");
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

  Heltec.display->clear();

  if (String(topic) == "p2lD") {
    nuevoNumeroDispositivo = message.toInt();
    esperandoConfirmacion = true;
    Heltec.display->drawString(0, 0, "Nuevo ID propuesto: " + message);
  } else if (String(topic) == "confirmchange" && esperandoConfirmacion) {
    if (message == "1") {
      preferences.putLong("dispositivo", nuevoNumeroDispositivo);
      esperandoConfirmacion = false;
      Heltec.display->drawString(0, 0, "ID cambiado a: " + String(nuevoNumeroDispositivo));
    }
  } else if (String(topic) == "displayvalue") {
    Heltec.display->drawString(0, 0, "Valor: " + message);
  }

  Heltec.display->display();
}

void setup() {
  Serial.begin(115200);
  // Inicializar el sistema y el OLED
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);

  pinMode(botonPin, INPUT_PULLUP); // Configura el pin del botón como entrada con pull-up

  preferences.begin("p2l", false);
  long numeroDispositivo = preferences.getLong("dispositivo", 1); // Por defecto es 1
  Serial.print("Número de dispositivo: ");
  Serial.println(numeroDispositivo);

  setupWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  static int lastButtonState = HIGH;
  int currentButtonState = digitalRead(botonPin);

  // Detecta el flanco descendente del botón (presionado)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    Serial.println("Botón presionado");
    // Publica un mensaje en "p2linput" indicando que el botón ha sido presionado
    client.publish("p2linput", "1");
    delay(200); // Debounce delay para evitar múltiples detecciones
  }
  lastButtonState = currentButtonState;
}

