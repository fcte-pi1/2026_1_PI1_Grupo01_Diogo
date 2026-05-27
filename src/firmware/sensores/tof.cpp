#include <Arduino.h>
#include <VL53L0X.h>
#include "tof.h"
#include "../config/pinos.h"

static VL53L0X sensores[NUM_TOF];

bool tofInit() {
    // 1. Colocar todos os XSHUT em saída e em LOW → todos os sensores em shutdown
    for (int i = 0; i < NUM_TOF; i++) {
        pinMode(TOF_XSHUT_PINS[i], OUTPUT);
        digitalWrite(TOF_XSHUT_PINS[i], LOW);
    }
    delay(10);

    // 2. Acordar cada sensor individualmente e reatribuir endereço único
    for (int i = 0; i < NUM_TOF; i++) {
        digitalWrite(TOF_XSHUT_PINS[i], HIGH);  // acorda apenas este sensor (ainda em 0x29)
        delay(10);

        sensores[i].setTimeout(500);
        if (!sensores[i].init()) {
            Serial.printf("[ToF] ERRO: sensor %d não inicializou (XSHUT GPIO %d)\n",
                          i, TOF_XSHUT_PINS[i]);
            return false;
        }

        sensores[i].setAddress(TOF_ADDRESSES[i]);
        sensores[i].startContinuous();

        Serial.printf("[ToF] Sensor %d OK → endereço 0x%02X (XSHUT GPIO %d)\n",
                      i, TOF_ADDRESSES[i], TOF_XSHUT_PINS[i]);
    }

    Serial.printf("[ToF] %d sensor(es) inicializado(s)\n", NUM_TOF);
    return true;
}

uint16_t tofLerDistancia(int i) {
    if (i < 0 || i >= NUM_TOF) return UINT16_MAX;
    return sensores[i].readRangeContinuousMillimeters();
}
