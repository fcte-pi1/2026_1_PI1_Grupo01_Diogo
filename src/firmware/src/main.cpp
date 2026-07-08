#include <Arduino.h>
#include <WiFi.h>
#include "config/pinos.h"
#include "sensores/i2c_bus.h"
#include "sensores/energia.h"
#include "sensores/tof.h"
#include "sensores/imu.h"
#include "atuadores/motores.h"
#include "navegacao/navegacao.h"
#include "navegacao/flood_fill.h"
#include "comunicacao/telemetria.h"

// =============================================================================
// MICROMOUSE — Navegação por FLOOD FILL com movimento controlado por PID.
//
// Sistema de coordenadas:
//   - Origem (0,0) no canto inferior-esquerdo.
//   - Robô começa em (0,0) olhando para o NORTE.
//   - x cresce para LESTE, y cresce para o NORTE.
//
// Fluxo por célula:
//   1. lê paredes (frente/esq/dir) e registra no mapa;
//   2. recalcula o flood fill;
//   3. se chegou ao objetivo, para;
//   4. escolhe o vizinho de menor distância e vai até ele.
// =============================================================================

FloodFill labirinto;

// Estado do robô no mapa.
uint8_t  robX = 0;
uint8_t  robY = 0;
Direcao  robDir = NORTE;

bool concluido = false;
bool iniciado  = false;   // trava de largada: só anda depois do comando 'g'

// --- Telemetria WiFi (mesma rede da bancada teste_navegacao) ---
const char*    WIFI_SSID  = "iPhone";
const char*    WIFI_PASS  = "06543210";
const uint16_t WIFI_PORTA = 8080;
WiFiServer server(WIFI_PORTA);
WiFiClient client;

void conectarWiFi();
void logLinha(const String& s);
bool lerComando(char& c);

// Converte direção relativa (frente/esq/dir) do robô em direção absoluta.
static inline Direcao dirFrente(Direcao h)  { return h; }
static inline Direcao dirDireita(Direcao h) { return (Direcao)((h + 1) & 3); }
static inline Direcao dirEsquerda(Direcao h){ return (Direcao)((h + 3) & 3); }

// Move a posição lógica uma célula na direção dada.
static void avancarPosicao(Direcao d) {
    switch (d) {
        case NORTE: robY++; break;
        case LESTE: robX++; break;
        case SUL:   robY--; break;
        case OESTE: robX--; break;
    }
}

// Lê os sensores e grava as paredes vistas na célula atual.
static void registrarParedes() {
    if (navParedeFrente())   labirinto.definirParede(robX, robY, dirFrente(robDir),  true);
    if (navParedeDireita())  labirinto.definirParede(robX, robY, dirDireita(robDir), true);
    if (navParedeEsquerda()) labirinto.definirParede(robX, robY, dirEsquerda(robDir),true);
}

// Gira o robô (física + estado lógico) para a direção absoluta desejada.
static void orientarPara(Direcao destino) {
    uint8_t diff = (destino - robDir) & 3;
    if (diff != 0) {
        // Plano A: se há parede À FRENTE (junção L/T), esquadra ANTES de girar —
        // encosta na parede -> esquadra o rumo + minimiza o offset do eixo -> a
        // roda traseira não raspa. Em cruzamento aberto, gira direto (giro limpo).
        if (navParedeFrente()) {
            if (!navEsquadrar()) logLinha("[MAIN] AVISO: esquadro nao tocou a parede da frente.");
        }
        switch (diff) {
            case 1: navGirarDireita();   break;
            case 2: navGirarMeiaVolta(); break;   // 180: Inc.4 troca por sair de ré
            case 3: navGirarEsquerda();  break;
        }
    }
    robDir = destino;
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== MICROMOUSE FLOOD FILL ===");

    i2cInit();

    if (!energiaInit()) {
        Serial.println("[MAIN] AVISO: INA219 nao inicializou; usando leitura de reserva.");
    }

    if (!imuInit())     { Serial.println("[MAIN] ERRO: IMU.");    while (true); }
    imuCalibrarOffsetZ(500);   // 500 = offset do giro menos ruidoso (casa com a bancada)
    imuZerarAnguloZ();

    if (!tofInit())     { Serial.println("[MAIN] ERRO: ToF.");    while (true); }
    if (!motoresInit()) { Serial.println("[MAIN] ERRO: Motores.");while (true); }
    if (!navInit())     { Serial.println("[MAIN] ERRO: Nav.");    while (true); }

    labirinto.iniciar();              // paredes zeradas + moldura + objetivo central
    navZerarRumo();

    // Sobe a telemetria (Wi-Fi + WebSocket) numa task no Core 0. A navegação
    // segue no loop() (Core 1) sem ser travada pela rede.
    telemetriaInit();

    conectarWiFi();
    logLinha("=== PRONTO: robo em (0,0) olhando NORTE. Envie 'g' para INICIAR. ===");
}

void loop() {
    // Aceita/atualiza o cliente de telemetria (nc <IP> 8080).
    if (server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            logLinha("=== Conectado. 'g' inicia a corrida | 'p' para. ===");
        } else {
            server.available().stop();   // já tem cliente: recusa o segundo
        }
    }

    // Comandos: 'g' inicia a corrida; 'p' para (só surte efeito ENTRE movimentos —
    // as primitivas são bloqueantes, então o 'p' fica no buffer e é lido ao fim da
    // célula atual; latência de ~1 célula. E-stop imediato fica p/ a opção 2).
    char c;
    while (lerComando(c)) {
        if ((c == 'g' || c == 'G') && !iniciado) {
            iniciado = true;
            logLinha("=== INICIANDO CORRIDA ===");
        } else if ((c == 'p' || c == 'P') && iniciado && !concluido) {
            logLinha("=== PARADO pelo 'p'. Reset p/ rodar de novo. ===");
            navParar();
            concluido = true;
        }
    }

    // Trava de largada: o robô fica PARADO até você mandar 'g'.
    if (!iniciado) {
        delay(20);
        return;
    }

    if (concluido) {
        navParar();
        motoresAtualizar();
        delay(50);
        return;
    }

    // 1. Mapeia o que o robô enxerga na célula atual.
    registrarParedes();

    // 2. Recalcula distâncias até o objetivo.
    labirinto.calcular();

    {
        char buf[96];
        snprintf(buf, sizeof(buf), "[POS] (%u,%u) rumo=%u  dist=%u",
                 robX, robY, robDir, labirinto.distancia(robX, robY));
        logLinha(buf);
    }

    // 3. Chegou ao centro?
    if (labirinto.ehObjetivo(robX, robY)) {
        logLinha("=== OBJETIVO ALCANCADO! ===");
        labirinto.imprimirSerial();   // mapa de distâncias (só Serial)
        navParar();
        concluido = true;
        // Estado terminal → backend finaliza a corrida como CONCLUIDA.
        telemetriaAtualizar(millis() - inicioCorridaMs, robX, robY, robDir,
                            "OBJETIVO_ENCONTRADO");
        return;
    }

    // 4. Decide o próximo passo pelo flood fill.
    Direcao proxima;
    if (!labirinto.melhorDirecao(robX, robY, robDir, proxima)) {
        logLinha("[MAIN] Preso: nenhuma direcao valida.");
        navParar();
        concluido = true;
        // Estado terminal → backend finaliza a corrida como NAO_CONCLUIDA.
        telemetriaAtualizar(millis() - inicioCorridaMs, robX, robY, robDir,
                            "ERRO");
        return;
    }

    // 5. Move para a próxima célula.
    uint8_t diff = (proxima - robDir) & 3;
    float cm = 0.0f;
    bool ok;
    if (diff == 2) {
        // Beco / meia-volta: SAI DE RÉ (não gira 180 no lugar — a frente comprida
        // varreria a parede lateral). Mantém o rumo; só a posição recua uma célula.
        ok = navAndarUmaCelula(true, &cm);
    } else {
        orientarPara(proxima);            // esquadro (se parede à frente) + giro c/ viés
        ok = navAndarUmaCelula(false, &cm); // saída: completa o resíduo + centraliza
    }
    {
        char buf[96];
        snprintf(buf, sizeof(buf), "[MOV] %s andou %.1f cm | rumo %.1f deg%s",
                 (diff == 2) ? "re" : "frente", cm, imuLerAnguloZ(), ok ? "" : "  <<< CURTO");
        logLinha(buf);
    }

    // Sem feedback de sucesso, um movimento incompleto (stall/jam) dessincronizaria o
    // mapa em silêncio. Melhor PARAR com diagnóstico do que decidir errado em cascata.
    if (!ok) {
        logLinha("[MAIN] Movimento incompleto -> parando p/ nao dessincronizar o mapa.");
        navParar();
        concluido = true;
        return;                           // NÃO avança a posição lógica
    }
    avancarPosicao(proxima);
}

// -----------------------------------------------------------------------------
// Telemetria WiFi
// -----------------------------------------------------------------------------
void conectarWiFi() {
    Serial.printf("[MAIN] Conectando em \"%s\" (DHCP) ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) {
        delay(300);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[MAIN] Wi-Fi OK. Conecte em  %s:%u\n",
                      WiFi.localIP().toString().c_str(), WIFI_PORTA);
        server.begin();
    } else {
        Serial.println("\n[MAIN] Wi-Fi FALHOU. Da p/ iniciar pelo Serial ('g') mesmo assim.");
    }
}

// Manda a linha pra Serial E pro cliente WiFi (se houver).
void logLinha(const String& s) {
    Serial.println(s);
    if (client && client.connected()) client.println(s);
}

// Lê 1 caractere do Serial ou do cliente WiFi. true se leu algo.
bool lerComando(char& c) {
    if (Serial.available() > 0) { c = (char)Serial.read(); return true; }
    if (client && client.connected() && client.available() > 0) {
        c = (char)client.read();
        return true;
    }
    return false;
}
