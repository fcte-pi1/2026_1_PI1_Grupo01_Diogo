#include <Arduino.h>
#include "config/pinos.h"
#include "sensores/i2c_bus.h"
#include "sensores/tof.h"
#include "sensores/imu.h"
#include "atuadores/motores.h"
#include "navegacao/navegacao.h"
#include "navegacao/flood_fill.h"

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
        if (navParedeFrente()) navEsquadrar();
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

    if (!imuInit())     { Serial.println("[MAIN] ERRO: IMU.");    while (true); }
    imuCalibrarOffsetZ(250);
    imuZerarAnguloZ();

    if (!tofInit())     { Serial.println("[MAIN] ERRO: ToF.");    while (true); }
    if (!motoresInit()) { Serial.println("[MAIN] ERRO: Motores.");while (true); }
    if (!navInit())     { Serial.println("[MAIN] ERRO: Nav.");    while (true); }

    labirinto.iniciar();              // paredes zeradas + moldura + objetivo central
    navZerarRumo();

    Serial.println("=== PRONTO: coloque o robô em (0,0) olhando p/ NORTE ===");
    delay(2000);
}

void loop() {
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

    Serial.printf("[POS] (%u,%u) rumo=%u  dist=%u\n",
                  robX, robY, robDir, labirinto.distancia(robX, robY));

    // 3. Chegou ao centro?
    if (labirinto.ehObjetivo(robX, robY)) {
        Serial.println("=== OBJETIVO ALCANCADO! ===");
        labirinto.imprimirSerial();
        navParar();
        concluido = true;
        return;
    }

    // 4. Decide o próximo passo pelo flood fill.
    Direcao proxima;
    if (!labirinto.melhorDirecao(robX, robY, robDir, proxima)) {
        Serial.println("[MAIN] Preso: nenhuma direção válida.");
        navParar();
        concluido = true;
        return;
    }

    // 5. Move para a próxima célula.
    uint8_t diff = (proxima - robDir) & 3;
    if (diff == 2) {
        // Beco / meia-volta: SAI DE RÉ (não gira 180 no lugar — a frente comprida
        // varreria a parede lateral). Mantém o rumo; só a posição recua uma célula.
        navAndarUmaCelula(true);
    } else {
        orientarPara(proxima);   // esquadro (se parede à frente) + giro c/ viés
        navAndarUmaCelula();     // saída: completa o resíduo + centraliza
    }
    avancarPosicao(proxima);
}
