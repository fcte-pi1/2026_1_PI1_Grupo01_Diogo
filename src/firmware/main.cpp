#include <Arduino.h>
#include "sensores/i2c_bus.h"
#include "sensores/imu.h"
#include "sensores/tof.h"
#include "config/pinos.h"

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Micromouse — Teste de Sensores (Issue #71) ===");

    i2cInit();
    i2cScan();

    if (!imuInit()) {
        Serial.println("[SETUP] Falha no IMU — verifique o hardware e reinicie");
    }

    if (!tofInit()) {
        Serial.println("[SETUP] Falha nos ToF — verifique o hardware e reinicie");
    }

    Serial.println("[SETUP] Iniciando leitura contínua...\n");
}

void loop() {
    float giroZ = imuLerGiroZ();

    Serial.printf("Z=%+6.1f dps | ToF:", giroZ);
    for (int i = 0; i < NUM_TOF; i++) {
        uint16_t d = tofLerDistancia(i);
        Serial.printf(" %4u", d);
    }
    Serial.println(" mm");

    delay(200);
}
