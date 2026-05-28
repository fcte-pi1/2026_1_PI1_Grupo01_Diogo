#include <Arduino.h>
#include "sensores/i2c_bus.h"
#include "sensores/imu.h"
#include "sensores/tof.h"
#include "atuadores/motores.h"
#include "movimentacao/movimentacao.h"
#include "config/pinos.h"

// ---------------------------------------------------------------------------
// Sequência de teste hardcoded — Issue #74
// Coloque o robô no chão, aguarde a contagem regressiva e observe:
//   1. Avança uma célula em linha reta
//   2. Gira 90° à direita
//   3. Avança uma célula
//   4. Gira 90° à esquerda
//   5. Para
// ---------------------------------------------------------------------------
static void executarTesteMovimentacao() {
    Serial.println("\n=== TESTE #74 — API de Movimentacao ===");
    Serial.println("Contagem regressiva: 3...");
    delay(1000);
    Serial.println("2...");
    delay(1000);
    Serial.println("1...");
    delay(1000);
    Serial.println("GO!\n");

    Serial.println("[Teste] Passo 1: avancarCelula");
    avancarCelula();
    delay(500);

    Serial.println("[Teste] Passo 2: girarDireita90");
    girarDireita90();
    delay(500);

    Serial.println("[Teste] Passo 3: avancarCelula");
    avancarCelula();
    delay(500);

    Serial.println("[Teste] Passo 4: girarEsquerda90");
    girarEsquerda90();
    delay(500);

    Serial.println("[Teste] Passo 5: pararMovimentacao");
    pararMovimentacao();

    Serial.println("\n=== Teste concluido ===");
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Micromouse — Issue #74: API de Movimentacao ===");

    i2cInit();
    i2cScan();

    if (!imuInit()) {
        Serial.println("[SETUP] Falha no IMU — verifique o hardware e reinicie");
    }

    imuCalibrarOffsetZ(200); // calibra offset do giroscópio Z (robô parado)

    if (!tofInit()) {
        Serial.println("[SETUP] Falha nos ToF — verifique o hardware e reinicie");
    }

    if (!motoresInit()) {
        Serial.println("[SETUP] Falha ao inicializar motores — verifique o TB6612FNG");
    }
    motoresParar();

    movimentacaoInit();

    executarTesteMovimentacao();

    Serial.println("\n[SETUP] Entrando em modo monitor (leitura de sensores)...\n");
}

void loop() {
    motoresAtualizar();

    float giroZ = imuLerGiroZ();

    Serial.printf("Z=%+6.1f dps | ToF:", giroZ);
    for (int i = 0; i < NUM_TOF; i++) {
        uint16_t d = tofLerDistancia(i);
        Serial.printf(" %4u", d);
    }
    Serial.println(" mm");

    delay(200);
}
