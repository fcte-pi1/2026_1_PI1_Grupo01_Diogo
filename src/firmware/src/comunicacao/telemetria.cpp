#include "comunicacao/telemetria.h"

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <string>

#include "config/rede.h"
#include "sensores/energia.h"
#include "sensores/tof.h"
#include "comunicacao/telemetria_contrato.h"

// =============================================================================
// Estado do módulo (privado ao arquivo). A montagem do JSON e as regras do
// contrato ficam no módulo PURO telemetria_contrato (testável no host).
//
// - snap:        snapshot compartilhado entre a navegação (Core 1, escreve) e a
//                task de telemetria (Core 0, lê). Protegido por mutexSnapshot.
// - wsConectado: escrito no callback do WebSocket (Core 0), lido na task.
// - enviarJa:    flag "envie agora" setada por telemetriaAtualizar() ao entrar
//                num estado terminal; consumida pela task.
// =============================================================================

static WebSocketsClient    webSocket;
static SemaphoreHandle_t   mutexSnapshot = nullptr;
static telemetria::Snapshot snap;
static volatile bool       wsConectado = false;
static volatile bool       enviarJa    = false;

// Copia o snapshot sob lock, serializa no contrato { type, payload } e envia.
static void enviarTelemetria() {
    telemetria::Snapshot s;
    if (xSemaphoreTake(mutexSnapshot, portMAX_DELAY) != pdTRUE) return;
    s = snap;
    xSemaphoreGive(mutexSnapshot);

    const std::string out = telemetria::montarEnvelope(s);
    webSocket.sendTXT(out.c_str());
}

static void onWsEvent(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            wsConectado = true;
            Serial.println("[WS] conectado ao backend");
            break;
        case WStype_DISCONNECTED:
            wsConectado = false;
            Serial.println("[WS] desconectado");
            break;
        default:
            break;   // não esperamos mensagens do servidor (send-only)
    }
}

static bool conectarWiFi() {
    Serial.printf("[WS] conectando Wi-Fi \"%s\"", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);                 // evita travadas do rádio no laço
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WS] Wi-Fi OK. IP=%s RSSI=%d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        return true;
    }
    Serial.println("\n[WS] Wi-Fi FALHOU (cheque SSID/senha e se e' 2,4 GHz).");
    return false;
}

// Task da telemetria — roda no Core 0, junto da pilha de rede.
static void tarefaTelemetria(void *param) {
    conectarWiFi();

    webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
    webSocket.onEvent(onWsEvent);
    webSocket.setReconnectInterval(WS_RECONNECT_MS);

    uint32_t ultimoEnvio = 0;
    for (;;) {
        webSocket.loop();   // precisa ser chamada com frequência (mantém a conexão)

        if (WiFi.status() != WL_CONNECTED) {
            conectarWiFi();  // reconecta o Wi-Fi; o WebSocket reconecta sozinho
        }

        const uint32_t agora = millis();
        const bool porTempo  = (agora - ultimoEnvio) >= TELEMETRIA_INTERVALO_MS;
        if (wsConectado && (enviarJa || porTempo)) {
            enviarTelemetria();
            ultimoEnvio = agora;
            enviarJa = false;
        }

        vTaskDelay(pdMS_TO_TICKS(5));   // cede a CPU (nunca segura o Core 0)
    }
}

void telemetriaInit() {
    mutexSnapshot = xSemaphoreCreateMutex();
    memset(&snap, 0, sizeof(snap));
    snap.estado     = "INICIANDO";
    snap.bateriaPct = energiaLerPct();

    // Fixa a task no Core 0 (o loop()/navegação fica no Core 1).
    xTaskCreatePinnedToCore(
        tarefaTelemetria,   // função
        "telemetria",       // nome
        8192,               // stack (bytes) — folgado p/ WiFi+WS+JSON
        nullptr,            // parâmetro
        1,                  // prioridade
        nullptr,            // handle
        0                   // CORE 0
    );
}

void telemetriaAtualizar(uint32_t tempoCorridaMs,
                         uint8_t x, uint8_t y, Direcao dir,
                         const char *estado) {
    // Leitura dos ToF acontece AQUI (Core 1 / dono do I²C), não na task.
    const uint16_t frente = tofLerDistancia(0);
    const uint16_t esq    = tofLerDistancia(1);
    const uint16_t dirD   = tofLerDistancia(2);

    if (xSemaphoreTake(mutexSnapshot, portMAX_DELAY) == pdTRUE) {
        snap.tempoCorridaMs = tempoCorridaMs;
        snap.x      = x;
        snap.y      = y;
        snap.dir    = dir;
        snap.estado = estado;   // sempre string literal (lifetime estático)
        snap.bateriaPct = energiaLerPct();
        snap.frenteMm = frente;
        snap.esqMm    = esq;
        snap.dirMm    = dirD;
        xSemaphoreGive(mutexSnapshot);
    }

    // Estado terminal força envio imediato (não espera o tick de 5 Hz).
    if (telemetria::estadoTerminal(estado)) enviarJa = true;
}
