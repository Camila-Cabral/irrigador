#include <WiFi.h>
#include <PubSubClient.h>

// Configurações de Wi-Fi
const char* ssid = "SSID da rede Wi-Fi";        // Substitua pelo SSID da sua rede Wi-Fi
const char* password = "Senha da rede Wi-Fi";   // Substitua pela senha da sua rede Wi-Fi

// Configurações do TagoIO (MQTT)
const char* mqttServer = "mqtt.tago.io";
const int mqttPort = 1883;
const char* mqttUser = "TagoIO-Token";     // Usuário MQTT (deixe como "TagoIO-Token") conforme a documentação do Tago.io
const char* mqttToken = "Token do bucket criado no tago.io"; // Substitua pelo token do seu bucket no TagoIO

WiFiClient espClient;
PubSubClient client(espClient);

const int nivelAguaPin = 34;
const int sensorUmidadePin = 35;
const int relayPin = 18;
const int maxSensorAgua = 1800;
const int maxSensorUmidade = 4095;

void setup() {
  Serial.begin(115200);
  
  pinMode(relayPin, OUTPUT);  // Configura o relé como saída

  // Conecta ao Wi-Fi
  connectWiFi();

  // Configura o cliente MQTT
  client.setServer(mqttServer, mqttPort);

  // Conecta ao TagoIO via MQTT
  connectMQTT();
}

void loop() {
  // Verifique se o Wi-Fi está desconectado e reconecte, se necessário
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado. Tentando reconectar...");
    connectWiFi();  // Reconecta ao Wi-Fi se desconectado
  }

  // Verifique se ainda está conectado ao MQTT, reconecte se necessário
  if (!client.connected()) {
    connectMQTT();  // Tenta reconectar ao TagoIO
  }
  client.loop();  // Mantém o loop do cliente MQTT

  // Leitura do sensor de nível de água
  int valorNivelAgua = analogRead(nivelAguaPin);
  if (valorNivelAgua > maxSensorAgua) {
    valorNivelAgua = maxSensorAgua;
  }
  float nivelAguaPercentual = (valorNivelAgua / (float)maxSensorAgua) * 100;

  // Leitura do sensor de umidade do solo
  int valorSensorUmidade = analogRead(sensorUmidadePin);
  float sensorUmidadePercentual = ((maxSensorUmidade - valorSensorUmidade) / (float)maxSensorUmidade) * 100;

  // Exibe os resultados no monitor serial
  Serial.print("Nível de água: ");
  Serial.print(nivelAguaPercentual);
  Serial.println("%");

  Serial.print("Umidade do solo: ");
  Serial.print(sensorUmidadePercentual);
  Serial.println("%");

  // Enviar dados para TagoIO
  sendTagoIOData("nivel_agua", nivelAguaPercentual);
  sendTagoIOData("umidade_solo", sensorUmidadePercentual);

  // Verifica condições para o relé
  if (nivelAguaPercentual > 30 && sensorUmidadePercentual < 30) {
    digitalWrite(relayPin, HIGH);  // Liga o relé
    Serial.println("Relé ligado (Condições atendidas)");
    sendTagoIOData("status_rele", 1);  // Envia status do relé
  } else {
    digitalWrite(relayPin, LOW);   // Desliga o relé
    Serial.println("Relé desligado (Condições não atendidas)");
    sendTagoIOData("status_rele", 0);  // Envia status do relé
  }

  delay(5000);  // Espera 1 minuto
}

// Função para conectar ao Wi-Fi
void connectWiFi() {
  Serial.print("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());  
}

// Função para conectar ao TagoIO via MQTT
void connectMQTT() {
  Serial.print("Conectando ao TagoIO...");
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqttUser, mqttToken)) {
      Serial.println("\nConectado ao TagoIO!");
    } else {
      Serial.print("Falha ao conectar ao TagoIO, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(3600);
    }
  }
}

// Função para enviar dados ao TagoIO
void sendTagoIOData(const char* variable, float value) {
  String payload = "{\"variable\":\"";
  payload += variable;
  payload += "\",\"value\":";
  payload += value;
  payload += "}";

  char attributes[100];
  payload.toCharArray(attributes, 100);
  client.publish("tago/data/post", attributes);
}
