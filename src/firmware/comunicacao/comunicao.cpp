#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "comunicao.h"


extern int posX;
extern int posY;
extern float batPct;
extern float anguloZ;
extern int distFre, distEsq, distDir;

const char* ssid = "NOME_DA_SUA_REDE_WIFI";
const char* password = "SENHA_DO_SEU_WIFI";
const char* serverName = "http://192.168.X.X:3000/telemetry"; 

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  
  Serial.print("Conectando ao Wi-Fi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWi-Fi Conectado com sucesso!");
  Serial.print("IP do Dispositivo: ");
  Serial.println(WiFi.localIP());
}

void verificarConexaoWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado! Tentando reconectar...");
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nReconectado com sucesso!");
    Serial.print("Novo IP: ");
    Serial.println(WiFi.localIP());
  }
}

void dispararTransmissaoWeb(const char* estado) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<400> doc;
    

    doc["tempo_corrida_ms"] = millis();
    doc["posicao_x"] = posX;
    doc["posicao_y"] = posY;
    doc["bateria_pct"] = batPct;

    doc["runId"] = "corrida-micromouse-v4";
    
    float yawN = fmodf(anguloZ, 360.0f);
    if (yawN < 0) yawN += 360.0f;
    doc["direcao_atual"] = (yawN < 45 || yawN > 315) ? "NORTE" :
                           (yawN >= 45 && yawN < 135) ? "LESTE" :
                           (yawN >= 135 && yawN < 225) ? "SUL" : "OESTE";
                           
    doc["estado_robo"] = estado;

    JsonObject leitura_sensores = doc.createNestedObject("leitura_sensores");
    leitura_sensores["dist_frente_cm"] = (distFre == -1) ? -1 : distFre / 10.0;
    leitura_sensores["dist_esquerda_cm"] = (distEsq == -1) ? -1 : distEsq / 10.0;
    leitura_sensores["dist_direita_cm"] = (distDir == -1) ? -1 : distDir / 10.0;

    String requestBody;
    serializeJson(doc, requestBody);

    Serial.println("\nEnviando dados para a API...");
    Serial.print("Payload: ");
    Serial.println(requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("HTTP Status Code: ");
      Serial.println(httpResponseCode);
      Serial.print("Resposta do Servidor: ");
      Serial.println(response);
    } else {
      Serial.print("Erro no envio do POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
}
