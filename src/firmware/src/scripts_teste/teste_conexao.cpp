#include <Arduino.h>
#include <WiFi.h>

// =============================================================================
// BANCADA DE CONEXÃO (Wi-Fi ISOLADO)
//
// Objetivo: descobrir COM CERTEZA se o problema é o rádio da ESP32 / a rede,
// ou o código de controle. Este teste NÃO usa motor, IMU nem laço de controle —
// só Wi-Fi.
//   - Se ESTE teste travar/cair  -> o problema é a placa / rede / ambiente.
//   - Se ESTE for liso mas o giro trava -> o problema é o Wi-Fi DENTRO do laço
//     de controle (aí a correção é tirar a leitura de rede do laço).
//
// O que faz:
//   - Conecta na Wi-Fi, imprime IP + RSSI no boot.
//   - Sobe servidor TCP na porta 8080.
//   - Quando um cliente conecta (nc <IP> 8080), TRANSMITE SOZINHO uma linha CSV
//     a cada ~INTERVALO_MS:  seq,millis,dt_ms,rssi_dbm,quedas
//   - dt_ms = tempo desde a última linha. Se passar de STALL_MS (~200 ms), o
//     loop TRAVOU — mesmo mecanismo que derruba o giro. Buracos = link ruim.
//   - Conta quedas de Wi-Fi e reconecta sozinho.
//
// Como usar:
//   1. Preencha WIFI_SSID / WIFI_PASS (do SEU hotspot, em 2,4 GHz).
//   2. Upload; pegue o IP no monitor serial (imprime IP + RSSI no boot).
//   3. No PC:   nc <IP> 8080 | tee teste_conexao.csv
//   4. Deixe rodar 30–60 s e Ctrl+C. Olhe: dt_ms grande, RSSI fraco, quedas > 0.
// =============================================================================

const char*    WIFI_SSID       = "iPhone";   // <-- PREENCHA (nome do hotspot)
const char*    WIFI_PASS       = "06543210";     // <-- PREENCHA (senha do hotspot)
const uint16_t WIFI_PORTA      = 8080;
const uint32_t WIFI_TIMEOUT_MS = 15000;
const uint16_t INTERVALO_MS    = 50;              // período de amostragem (~20 Hz)
const uint16_t STALL_MS        = 200;             // dt acima disso = travada do loop

WiFiServer server(WIFI_PORTA);
WiFiClient client;

uint32_t quedasWiFi = 0;    // quantas vezes o Wi-Fi caiu / não estava conectado
uint32_t seq        = 0;    // contador de linhas (buracos na sequência = perda)
uint32_t ultimoTick = 0;
uint32_t dtMaxMs    = 0;    // maior intervalo entre linhas (pior travada)
uint32_t stalls     = 0;    // quantos ticks passaram de STALL_MS

bool conectarWiFi() {
    Serial.printf("[CONEXAO] Conectando em \"%s\" ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[CONEXAO] Wi-Fi OK. IP: %s  RSSI: %d dBm  (porta %u)\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI(), WIFI_PORTA);
        server.begin();
        return true;
    }
    Serial.println("\n[CONEXAO] Wi-Fi FALHOU (cheque SSID/senha e se o hotspot é 2,4 GHz).");
    return false;
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== BANCADA DE CONEXAO (Wi-Fi isolado) ===");
    conectarWiFi();
    Serial.println("[CONEXAO] Conecte no PC:  nc <IP> 8080  (transmite sozinho, sem comando)");
    ultimoTick = millis();
}

void loop() {
    // Reconecta se o Wi-Fi caiu (e conta a queda).
    if (WiFi.status() != WL_CONNECTED) {
        quedasWiFi++;
        Serial.printf("[CONEXAO] Wi-Fi caiu/desconectado (evento #%lu). Reconectando...\n",
                      (unsigned long)quedasWiFi);
        conectarWiFi();
        ultimoTick = millis();
        return;
    }

    // Aceita/atualiza cliente TCP.
    if (server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            client.println("seq,millis,dt_ms,rssi_dbm,quedas");   // cabeçalho CSV
            seq = 0; dtMaxMs = 0; stalls = 0;                     // zera a janela de medição
        } else {
            server.available().stop();
        }
    }

    // Pacing ~INTERVALO_MS. O dt real revela travadas do loop (culpa do Wi-Fi).
    uint32_t agora = millis();
    uint32_t dt = agora - ultimoTick;
    if (dt < INTERVALO_MS) { delay(1); return; }
    ultimoTick = agora;

    if (dt > dtMaxMs) dtMaxMs = dt;
    if (dt > STALL_MS) stalls++;

    // Transmite uma linha (só quando há cliente conectado).
    if (client && client.connected()) {
        char linha[80];
        snprintf(linha, sizeof(linha), "%lu,%lu,%lu,%d,%lu",
                 (unsigned long)seq, (unsigned long)agora, (unsigned long)dt,
                 WiFi.RSSI(), (unsigned long)quedasWiFi);
        client.println(linha);
        seq++;
    }

    // Resumo periódico no Serial (~2 s) pra acompanhar até sem o PC.
    static uint32_t ultimoResumo = 0;
    if (agora - ultimoResumo > 2000) {
        ultimoResumo = agora;
        Serial.printf("[CONEXAO] seq=%lu  RSSI=%d dBm  dtMax=%lu ms  stalls=%lu  quedas=%lu\n",
                      (unsigned long)seq, WiFi.RSSI(), (unsigned long)dtMaxMs,
                      (unsigned long)stalls, (unsigned long)quedasWiFi);
    }
}
