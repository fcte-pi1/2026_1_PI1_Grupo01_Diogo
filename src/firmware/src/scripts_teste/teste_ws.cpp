#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#include "config/rede.h"

// =============================================================================
// BANCADA DE WEBSOCKET (isolada) — valida a ponta firmware -> backend SEM
// sensores nem navegação. Espelha o simulate-websocket.ts.
//
// O que faz:
//   - Conecta no Wi-Fi e abre um WebSocket como ROBÔ (?role=robo).
//   - Envia telemetria falsa no envelope { type:"telemetria", payload:{...} }
//     a ~5 Hz. A partir do 8º pacote envia estado terminal OBJETIVO_ENCONTRADO
//     (o backend deve finalizar a corrida como CONCLUIDA) e para.
//
// Como usar:
//   1. Suba o backend (porta 3000) e preencha config/rede.h (SSID/senha/WS_HOST).
//   2. pio run -e teste_ws -t upload -t monitor
//   3. Confira no log do backend / no frontend a telemetria chegando e a
//      corrida virando CONCLUIDA no fim.
// =============================================================================

static WebSocketsClient webSocket;
static volatile bool    conectado = false;
static uint32_t         contador  = 0;
static bool             encerrado = false;

static void onWsEvent(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            conectado = true;
            Serial.println("[TESTE_WS] conectado ao backend");
            break;
        case WStype_DISCONNECTED:
            conectado = false;
            Serial.println("[TESTE_WS] desconectado");
            break;
        case WStype_TEXT:
            Serial.printf("[TESTE_WS] recebido: %.*s\n", (int)length, payload);
            break;
        default:
            break;
    }
}

static void conectarWiFi() {
    Serial.printf("[TESTE_WS] Wi-Fi \"%s\"", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED)
        Serial.printf("\n[TESTE_WS] IP=%s\n", WiFi.localIP().toString().c_str());
    else
        Serial.println("\n[TESTE_WS] Wi-Fi FALHOU");
}

static void enviarPacote() {
    contador++;
    const bool terminal = contador >= 8;

    JsonDocument doc;
    doc["type"] = "telemetria";
    JsonObject p = doc["payload"].to<JsonObject>();
    p["tempo_corrida_ms"] = contador * 1000;
    p["posicao_x"]        = contador;
    p["posicao_y"]        = contador % 5;
    p["direcao_atual"]    = "NORTE";
    p["estado_robo"]      = terminal ? "OBJETIVO_ENCONTRADO" : "EXPLORANDO";
    p["bateria_pct"]      = 100 - (int)contador;

    JsonObject sensores = p["leitura_sensores"].to<JsonObject>();
    sensores["dist_frente_cm"]   = 12.0 + contador;
    sensores["dist_esquerda_cm"] = 4.0;
    sensores["dist_direita_cm"]  = 7.0;

    String out;
    serializeJson(doc, out);
    webSocket.sendTXT(out);
    Serial.printf("[TESTE_WS] enviado pacote %lu%s\n",
                  (unsigned long)contador, terminal ? " (terminal)" : "");

    if (terminal) encerrado = true;
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== BANCADA WEBSOCKET (isolada) ===");
    conectarWiFi();
    webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
    webSocket.onEvent(onWsEvent);
    webSocket.setReconnectInterval(WS_RECONNECT_MS);
}

void loop() {
    webSocket.loop();

    static uint32_t ultimo = 0;
    if (!encerrado && conectado && millis() - ultimo >= TELEMETRIA_INTERVALO_MS) {
        ultimo = millis();
        enviarPacote();
    }
}
