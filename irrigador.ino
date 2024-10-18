const int nivelAguaPin = 12;  // Pino analógico conectado ao sensor de nível de água
const int sensorUmidadePin = 35;  // Pino analógico conectado ao sensor de umidade do solo
const int relayPin = 18;  // Pino conectado à entrada N1 do módulo de relé
const int maxSensorAgua = 1800;  // Valor máximo do sensor de nível de água (quando totalmente submerso)
const int maxSensorUmidade = 4095;  // Valor máximo do sensor de umidade (solo totalmente seco em 4095)

void setup() {
  Serial.begin(115200);  // Inicializa a comunicação serial com velocidade de 115200 bps
  pinMode(relayPin, OUTPUT);  // Configura o pino do relé como saída para controlar o relé
}

void loop() {
  // Leitura do sensor de nível de água
  // Realiza a leitura analógica do sensor de nível de água no pino definido
  int valorNivelAgua = analogRead(nivelAguaPin);

  // Verifica se o valor lido ultrapassa o máximo esperado e, se necessário, ajusta ao valor máximo
  if (valorNivelAgua > maxSensorAgua) {
    valorNivelAgua = maxSensorAgua;
  }

  // Converte o valor lido para percentual (0% a 100%), considerando o valor máximo definido (1800)
  float nivelAguaPercentual = (valorNivelAgua / (float)maxSensorAgua) * 100;

  // Leitura do sensor de umidade do solo
  // Realiza a leitura analógica do sensor de umidade no pino definido
  int valorSensorUmidade = analogRead(sensorUmidadePin);

  // Converte o valor lido para percentual (0% = solo inundado, 100% = solo seco)
  // A fórmula inverte o valor para que 0% seja quando o solo está muito úmido e 100% quando está totalmente seco
  float sensorUmidadePercentual = ((maxSensorUmidade - valorSensorUmidade) / (float)maxSensorUmidade) * 100;

  // Exibe os valores de nível de água e umidade do solo no monitor serial
  Serial.print("Nível de água: ");
  Serial.print(nivelAguaPercentual);  // Exibe o percentual do nível de água
  Serial.println("%");

  Serial.print("Umidade do solo: ");
  Serial.print(sensorUmidadePercentual);  // Exibe o percentual de umidade do solo
  Serial.println("%");

  // Verifica as condições para ativar ou desativar o relé:
  // Se o nível de água for maior que 30% E a umidade do solo for menor que 30%
  if (nivelAguaPercentual > 30 && sensorUmidadePercentual < 30) {
    digitalWrite(relayPin, HIGH);  // Ativa o relé se as condições forem atendidas
    Serial.println("Relé ligado (Condições atendidas)");  // Exibe mensagem indicando que o relé foi ligado
  } else {
    digitalWrite(relayPin, LOW);   // Desativa o relé se as condições não forem atendidas
    Serial.println("Relé desligado (Condições não atendidas)");  // Exibe mensagem indicando que o relé foi desligado
  }

  delay(60000);  // Aguarda 1 segundo antes de repetir o processo de leitura e verificação
}
