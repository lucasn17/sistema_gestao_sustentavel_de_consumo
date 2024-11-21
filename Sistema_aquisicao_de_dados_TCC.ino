#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include "EmonLib.h"

// Substitua com suas credenciais de rede
const char* ssid = "LUCAS";
const char* password = "17121997";

// URL do servidor
const char* serverName = "http://192.168.100.5/chuveiro_comercial/post-esp-data.php";

// Chave API
String apiKeyValue = "tPmAT5Ab3j7F9";
String sensorLocation = "Banheiro1";

#define SS_PIN 5       // Pino de seleção do chip
#define RST_PIN 17     // Pino de reset
#define LED_PIN 12     // Pino do LED
#define SENSOR_PIN 25  // Pino do sensor de fluxo
#define SCT_PIN 22     // Pino do sensor SCT013

MFRC522 rfid(SS_PIN, RST_PIN);  // Cria uma instância do MFRC522
EnergyMonitor SCT013;           // Instância do sensor SCT013
unsigned long startTime;        // Tempo de início
bool processActive = false;     // Flag para verificar se o processo está ativo
String elapsedTimeString;       // Tempo formatado
String value3;                  // Para armazenar o tempo formatado
String value4;                  // Para armazenar o valor total
volatile int pulseCount = 0;    // Contador de pulsos
float flowRate = 0.0;           // Vazão em litros por minuto
float volume = 0.0;             // Volume em litros
int tensao = 220;               // Tensão em volts
float energia = 0.0;            // Energia calculada em kWh

void setup() {
  Serial.begin(115200);                                                       // Inicia a comunicação serial
  connectToWiFi();                                                            // Conecta ao Wi-Fi
  SPI.begin();                                                                // Inicia a comunicação SPI
  rfid.PCD_Init();                                                            // Inicializa o leitor RFID
  SCT013.current(SCT_PIN, 0.15);                                              // Inicializa o sensor SCT013
  pinMode(LED_PIN, OUTPUT);                                                   // Define o pino do LED como saída
  digitalWrite(LED_PIN, HIGH);                                                // Garante que o LED esteja apagado (sinal alto)
  pinMode(SENSOR_PIN, INPUT_PULLUP);                                          // Configura o pino do sensor como entrada
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), pulseCounter, FALLING);  // Configura a interrupção para contar os pulsos
}

void loop() {
  checkWiFiConnection();  // Verifica a conexão Wi-Fi

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (checkCardID()) {
      if (!processActive) {
        // Início do processo
        digitalWrite(LED_PIN, LOW);  // Acende o LED (sinal baixo)
        startTime = millis();         // Armazena o tempo de início
        processActive = true;         // Ativa o processo
        Serial.println("Processo iniciado.");
      } else {
        // Fim do processo
        digitalWrite(LED_PIN, HIGH);                                 // Apaga o LED (sinal alto)
        unsigned long elapsedTime = (millis() - startTime) / 1000;  // Tempo decorrido em segundos

        // Cálculo de horas, minutos e segundos
        unsigned long hours = elapsedTime / 3600;           // Horas
        unsigned long minutes = (elapsedTime % 3600) / 60;  // Minutos
        unsigned long seconds = elapsedTime % 60;           // Segundos restantes

        // Calcula o volume
        float litrosPorPulso = 1.0 / 450.0;  // Exemplo: 450 pulsos por litro
        volume = pulseCount * litrosPorPulso;  // Calcula o volume em litros

        // Calcula a energia
        float corrente = SCT013.calcIrms(1480);  // Calcula a corrente RMS
        energia += (corrente * tensao) / 3600000;  // Adiciona energia em kWh

        // Calcul a o valor total
        float valorTotal = (volume * 0.0012) + (energia * 0.6);
        value4 = String(valorTotal);  // Atribui o valor total à variável value4

        // Formata o tempo em "HH:MM:SS"
        elapsedTimeString = String(hours < 10 ? "0" : "") + String(hours) + ":" + String(minutes < 10 ? "0" : "") + String(minutes) + ":" + String(seconds < 10 ? "0" : "") + String(seconds);
        value3 = elapsedTimeString; // Atribui o tempo formatado à variável value3

        // Impressão dos resultados
        Serial.println("Processo finalizado.");
        Serial.print("Tempo (HH:MM:SS): ");
        Serial.print(elapsedTimeString);  // Tempo 
        Serial.println("  ");
        Serial.print("Volume (Litros): ");
        Serial.print(volume);
        Serial.println(" L");  // Volume
        Serial.print("Energia elétrica (Kilo Watts/Hora): ");
        Serial.print(energia);
        Serial.println(" kWh");  // Energia em Kilo Watts hora
        Serial.print("Valor Total: ");
        Serial.print(valorTotal);
        Serial.println(" R$");  // Valor Total

        // Envia os dados para o servidor
        sendDataToServer(elapsedTimeString, volume, energia, valorTotal);

        pulseCount = 0;  // Zera o contador de pulsos
        volume = 0.0;    // Opcional: Zera o volume para garantir que comece do zero

        processActive = false;  // Reinicia o processo
      }
    }
    rfid.PICC_HaltA();       // Para a leitura do cartão
    rfid.PCD_StopCrypto1();  // Para a criptografia
  }
}

void pulseCounter() {
  pulseCount++;
}

void connectToWiFi() {
  Serial.print("Conectando ao WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" Conectado ao WiFi!");
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, tentando reconectar...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 5) {
      delay(1000);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconectado ao WiFi!");
    } else {
      Serial.println("Falha ao reconectar ao WiFi.");
    }
  }
}

void sendDataToServer(String elapsedTimeString, float volume, float energia, float valorTotal) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Inicia o HTTP
    http.begin(serverName);

    // Especifica o cabeçalho do tipo de conteúdo
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Prepara os dados para envio
    String cardID = "93 53 8D 9B"; // ID do cartão RFID
    String httpRequestData = "api_key=" + apiKeyValue + "&location=" + sensorLocation 
                             + "&value1=" + String(volume)
                             + "&value2=" + String(energia)
                             + "&value3=" + elapsedTimeString
                             + "&value4=" + String(valorTotal)
                             + "&card_id=" + cardID; // Adiciona o ID do cartão RFID

    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);

    // Envia a requisição HTTP POST
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      Serial.print("Código de resposta HTTP: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Código de erro: ");
      Serial.println(httpResponseCode);
      Serial.print("Mensagem de erro: ");
      Serial.println(http.errorToString(httpResponseCode));  // Adiciona esta linha para detalhes do erro
    }

    // Libera recursos
    http.end();
  } else {
    Serial.println("WiFi Desconectado");
  }
}

bool checkCardID() {
  // ID do cartão RFID esperado
  byte expectedID[5] = { 0x93, 0x53, 0x8D, 0x9B };
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != expectedID[i]) {
      return false;  // ID não corresponde
    }
  }
  return true;  // ID corresponde
}