#include <Arduino.h>
#include <WiFi.h>
#include "sensores/i2c_bus.h"
#include "sensores/tof.h"

// =============================================================================
// TESTE DE ToF (Wi-Fi) — ver as 3 distâncias ao vivo, ANTES da centralização.
//
// Objetivo: com o robô PARADO NO CENTRO do corredor, entender o que os sensores
// retornam de verdade. Interessa:
//   - esq e dir BATEM quando o robô está no meio? (senão, o "centro" é viesado)
//   - o valor CRU da direita vs o CORRIGIDO (-50 mm): a correção ajuda ou atrapalha?
//   - aparece leitura FALSA? (pula, trava num valor, número absurdo)
//
// Uso:  nc <IP> 8080   (transmite sozinho, ~5 linhas/s; o IP sai no serial no boot)
//   Colunas: frente_mm, esq_mm, dir_corr_mm, dir_bruta_mm, dif(esq-dir)
//   - esq_mm e frente_mm são crus (só a direita tem correção no código).
//   - centrado ideal: dif ~ 0 (esq ≈ dir_corr).
// =============================================================================

const char*    WIFI_SSID      = "VIVOFIBRA-8681";    // <-- coloque o nome da rede/hotspot
const char*    WIFI_PASS      = "DSCBCkm8r3";   // <-- coloque a senha
const uint16_t WIFI_PORTA      = 8080;
const uint32_t WIFI_TIMEOUT_MS = 8000;
WiFiServer server(WIFI_PORTA);
WiFiClient client;

const uint16_t INTERVALO_MS = 200;   // ~5 leituras por segundo

void conectarWiFi();
void logLinha(const String& s);

void setup() {
    Serial.begin(115200);
    delay(200);
    i2cInit();
    if (!tofInit()) {
        Serial.println("[TOF] ERRO: ToF nao inicializou. Travado.");
        while (true) delay(1000);
    }
    conectarWiFi();
    logLinha("[TOF] Pronto. Coloque o robo no CENTRO e compare esq vs dir.");
    logLinha("frente_mm,esq_mm,dir_corr_mm,dir_bruta_mm,dif_esq_dir");
}

void loop() {
    if (server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            logLinha("=== Conectado ao teste de ToF ===");
            logLinha("frente_mm,esq_mm,dir_corr_mm,dir_bruta_mm,dif_esq_dir");
        } else {
            server.available().stop();
        }
    }

    static uint32_t ultimo = 0;
    if (millis() - ultimo >= INTERVALO_MS) {
        ultimo = millis();
        const uint16_t frente   = tofLerDistancia(0);       // frente (cru)
        const uint16_t esq      = tofLerDistancia(1);       // esquerda (cru)
        const uint16_t dirCorr  = tofLerDistancia(2);       // direita (com -50)
        const uint16_t dirBruta = tofLerDistanciaBruta(2);  // direita (cru)
        const int32_t  dif      = (int32_t)esq - (int32_t)dirCorr;
        char linha[96];
        snprintf(linha, sizeof(linha), "%u,%u,%u,%u,%ld",
                 frente, esq, dirCorr, dirBruta, (long)dif);
        logLinha(String(linha));
    }
}

void logLinha(const String& s) {
    Serial.println(s);
    if (client && client.connected()) client.println(s);
}

void conectarWiFi() {
    Serial.printf("[TOF] Conectando em \"%s\" (DHCP) ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[TOF] Wi-Fi OK. Conecte em  %s:%u\n",
                      WiFi.localIP().toString().c_str(), WIFI_PORTA);
        server.begin();
    } else {
        Serial.println("\n[TOF] Wi-Fi FALHOU. Cheque SSID/senha e se o hotspot e 2,4GHz.");
    }
}
