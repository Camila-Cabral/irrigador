#include <WiFi.h>
#include <PubSubClient.h>

// Configurações de Wi-Fi
const char* ssid = "SSID";        // Substitua pelo nome da sua rede Wi-Fi
const char* password = "SENHA DO WIFI";   // Substitua pela senha da sua rede Wi-Fi

// Configurações do Thinger.io (MQTT)
const char* mqttServer = "backend.thinger.io";
const int mqttPort = 1883;
const char* mqttUser = "USUARIO DO THINGER.IO";      // Usuário MQTT (seu e-mail no Thinger.io)
const char* mqttToken = "SENHA";    // Senha do Thinger.io
const char* deviceID = "ID DO DEVICE";    // ID do dispositivo criado no Thinger.io

WiFiClient espClient;
PubSubClient client(espClient);

const int nivelAguaPin = 34;
const int sensorUmidadePin = 35;
const int relayPin = 18;
const int maxSensorAgua = 1800;
const int valorMaxSeco = 4095;    // Valor máximo quando o solo está seco
const int valorMinUmido = 2103;   // Valor mínimo quando o solo está totalmente úmido

void setup() {
  Serial.begin(115200);
  
  pinMode(relayPin, OUTPUT);  // Configura o relé como saída

  // Conecta ao Wi-Fi
  connectWiFi();

  // Configura o cliente MQTT
  client.setServer(mqttServer, mqttPort);

  // Conecta ao Thinger.io via MQTT
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
    connectMQTT();  // Tenta reconectar ao Thinger.io
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
  float sensorUmidadePercentual = ((valorMaxSeco - valorSensorUmidade) / (float)(valorMaxSeco - valorMinUmido)) * 100;

  // Garantia de que a umidade fique entre 0% e 100%
  if (sensorUmidadePercentual > 100) {
    sensorUmidadePercentual = 100;
  } else if (sensorUmidadePercentual < 0) {
    sensorUmidadePercentual = 0;
  }

  // Exibe os resultados no monitor serial
  Serial.print("Nível de água: ");
  Serial.print(nivelAguaPercentual);
  Serial.println("%");

  Serial.print("Umidade do solo: ");
  Serial.print(sensorUmidadePercentual);
  Serial.println("%");

  // Enviar dados para Thinger.io
  sendThingerData("nivel_agua", nivelAguaPercentual);
  sendThingerData("umidade_solo", sensorUmidadePercentual);

  // Verifica condições para o relé
  if (nivelAguaPercentual > 30 && sensorUmidadePercentual < 30) {
    digitalWrite(relayPin, HIGH);  // Liga o relé
    Serial.println("Relé ligado (Condições atendidas)");
    sendThingerData("status_rele", 1);  // Envia o estado do relé (ligado) para o Thinger.io
  } else {
    digitalWrite(relayPin, LOW);   // Desliga o relé
    Serial.println("Relé desligado (Condições não atendidas)");
    sendThingerData("status_rele", 0);  // Envia o estado do relé (desligado) para o Thinger.io
  }

  delay(60000);  // Espera 1 minuto antes de enviar os dados novamente
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

// Função para conectar ao Thinger.io via MQTT
void connectMQTT() {
  Serial.print("Conectando ao Thinger.io...");
  while (!client.connected()) {
    if (client.connect(deviceID, mqttUser, mqttToken)) {
      Serial.println("\nConectado ao Thinger.io!");
    } else {
      Serial.print("Falha ao conectar ao Thinger.io, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 60 segundos...");
      delay(60000);
    }
  }
}

// Função para enviar dados ao Thinger.io
void sendThingerData(const char* variable, float value) {
  String topic;

  // Definindo o tópico de acordo com a variável
  if (strcmp(variable, "umidade_solo") == 0) {
    topic = String(deviceID) + "/umidade_solo";  // Tópico para umidade do solo
  } else if (strcmp(variable, "nivel_agua") == 0) {
    topic = String(deviceID) + "/nivel_agua";   // Tópico para nível de água
  } else if (strcmp(variable, "status_rele") == 0) {
    topic = String(deviceID) + "/estado_rele";  // Tópico para o estado do relé
  } else {
    topic = String(deviceID) + "/telemetry";    // Tópico genérico para outros dados
  }

  // Prepara o payload com o nome da variável e o valor
  String payload = "{\"" + String(variable) + "\":" + String(value) + "}";

  char attributes[100];
  payload.toCharArray(attributes, 100);

  // Publica os dados no tópico correspondente
  client.publish(topic.c_str(), attributes);
}
