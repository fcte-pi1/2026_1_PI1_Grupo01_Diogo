#include <Arduino.h>
#include <WiFi.h>
#include "sensores/encoders.h"
#include "atuadores/motores.h"

// =============================================================================
// TESTE DE ENCODERS / ODOMETRIA (Wi-Fi) — calibra a CIRCUNFERENCIA_CM
//
// Empurre o robo uma distancia MEDIDA (ex.: 50 cm) e leia os pulsos + os cm
// reportados. Compare com a regua e ajuste CIRCUNFERENCIA_CM em encoders.cpp.
//
// Feito pra rodar SO pelo Wi-Fi (IP fixo), pra voce empurrar o robo solto,
// sem o cabo atrapalhar a linha reta. Os motores ficam desligados; as rodas
// rolam livres enquanto voce empurra.
//
// Uso:
//   1. nc 172.20.10.5 8080   (transmite sozinho, ~5 linhas/s)
//   2. alinhe o robo no inicio e mande 'z' (zera os contadores)
//   3. empurre RETO pela distancia medida (colado numa regua/parede)
//   4. pare e leia a ULTIMA linha: esq_cm, dir_cm (ficam estaveis parado)
//
//   Colunas: esq_pulsos,dir_pulsos,esq_cm,dir_cm,media_cm
//   Comando: z = zera os contadores
// =============================================================================

const char*    WIFI_SSID = "iPhone";    // <-- nome da rede/hotspot
const char*    WIFI_PASS = "06543210";   // <-- senha
IPAddress      IP_ESTATICO(172, 20, 10, 5);   // <-- CONECTE SEMPRE NESTE IP
IPAddress      GATEWAY    (172, 20, 10, 1);
IPAddress      SUBNET     (255, 255, 255, 240);
const uint16_t WIFI_PORTA      = 8080;
const uint32_t WIFI_TIMEOUT_MS = 8000;
WiFiServer server(WIFI_PORTA);
WiFiClient client;

const uint16_t INTERVALO_MS = 200;   // 5 leituras por segundo

void conectarWiFi();
bool lerComando(char& c);
void logLinha(const String& s);

void setup() {
    Serial.begin(115200);
    delay(200);
    // Poe o driver TB6612 em STANDBY pra as DUAS rodas ficarem LIVRES.
    // Sem isso os pinos do driver flutuam e um canal pode frear a roda,
    // arrastando-a no empurrao e corrompendo a contagem do encoder.
    motoresInit();
    motoresHabilitar(false);
    encodersInit();
    conectarWiFi();
    logLinha("[ENC] Pronto. 'z' zera no inicio; empurre reto; leia esq_cm/dir_cm.");
    logLinha("esq_pulsos,dir_pulsos,esq_cm,dir_cm,media_cm");
}

void loop() {
    if (server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            logLinha("=== Conectado ao teste de ENCODERS ===");
            logLinha("'z' zera. Colunas: esq_pulsos,dir_pulsos,esq_cm,dir_cm,media_cm");
        } else {
            server.available().stop();
        }
    }

    char c;
    if (lerComando(c) && (c == 'z' || c == 'Z')) {
        encodersZerar();
        logLinha("[ENC] >>> contadores ZERADOS <<<");
    }

    static uint32_t ultimo = 0;
    if (millis() - ultimo >= INTERVALO_MS) {
        ultimo = millis();
        const int32_t pe = encoderLerEsquerdo();
        const int32_t pd = encoderLerDireito();
        const float   ce = encoderDistanciaEsquerdaCm();
        const float   cd = encoderDistanciaDireitaCm();
        const float   media = 0.5f * (ce + cd);
        char linha[100];
        snprintf(linha, sizeof(linha), "%ld,%ld,%.2f,%.2f,%.2f",
                 (long)pe, (long)pd, ce, cd, media);
        logLinha(String(linha));
    }
}

bool lerComando(char& c) {
    if (Serial.available() > 0) { c = (char)Serial.read(); return true; }
    if (client && client.connected() && client.available() > 0) {
        c = (char)client.read();
        return true;
    }
    return false;
}

void logLinha(const String& s) {
    Serial.println(s);
    if (client && client.connected()) client.println(s);
}

void conectarWiFi() {
    Serial.printf("[ENC] Conectando em \"%s\" com IP fixo %s ...\n",
                  WIFI_SSID, IP_ESTATICO.toString().c_str());
    WiFi.mode(WIFI_STA);
    if (!WiFi.config(IP_ESTATICO, GATEWAY, SUBNET, GATEWAY)) {
        Serial.println("[ENC] WiFi.config falhou (verifique a faixa de IP).");
    }
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[ENC] Wi-Fi OK. Conecte em  %s:%u\n",
                      WiFi.localIP().toString().c_str(), WIFI_PORTA);
        server.begin();
    } else {
        Serial.println("\n[ENC] Wi-Fi FALHOU. Cheque SSID/senha e se o hotspot e 2,4GHz.");
    }
}
