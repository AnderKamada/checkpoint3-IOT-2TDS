#include <WiFi.h>
#include <PubSubClient.h>

const char* ID        = "grupo-fiap-iot";
const char* moduleID  = "esp32-q4";

// Wi-Fi
const char* SSID      = "Wokwi-GUEST";
const char* PASSWORD  = "";

// MQTT Broker
const char* BROKER_MQTT  = "4.201.104.101"; 
const int   BROKER_PORT  = 1883;
const char* mqttUser = "azureuser";       
const char* mqttPassword = "cp3mqttesp32"; 

#define TOPICO_PUBLISH  "esp32/sensores"

WiFiClient espClient;
PubSubClient client(espClient);

float gerarAleatorio(float min, float max) {
  return min + ((float)random(0, 10000) / 10000) * (max - min);
}

void conectarWiFi() {
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
}

void conectarMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT... ");
    if (client.connect(ID, mqttUser, mqttPassword)) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Erro: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Iniciando ESP32...");
  randomSeed(analogRead(0));
  conectarWiFi();
  client.setServer(BROKER_MQTT, BROKER_PORT);
  conectarMQTT();
}

void loop() {
  float temperatura = gerarAleatorio(20.0, 35.0);
  float umidade     = gerarAleatorio(40.0, 80.0);
  float pressao     = gerarAleatorio(980.0, 1050.0);
  float altitude    = gerarAleatorio(0.0, 500.0);

  String payload = String("{\"temperatura\":") + temperatura +
                   ", \"umidade\":" + umidade +
                   ", \"pressao\":" + pressao +
                   ", \"altitude\":" + altitude + "}";

  client.publish(TOPICO_PUBLISH, payload.c_str());

  Serial.println("Dados enviados: " + payload);

  delay(5000);
}
