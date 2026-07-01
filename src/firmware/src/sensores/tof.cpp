#include <Arduino.h>
#include <VL53L0X.h>
#include "tof.h"
#include "../config/pinos.h"

static VL53L0X sensores[NUM_TOF];

bool tofInit() {
    for (int i = 0; i < NUM_TOF; i++) {
        pinMode(TOF_XSHUT_PINS[i], OUTPUT);
        digitalWrite(TOF_XSHUT_PINS[i], LOW);
    }
    delay(10);

    for (int i = 0; i < NUM_TOF; i++) {
        digitalWrite(TOF_XSHUT_PINS[i], HIGH);  
        delay(15);

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
    
    uint16_t distanciaBruta = sensores[i].readRangeContinuousMillimeters();

    if (i == 2) {
        if (distanciaBruta > 2000) return distanciaBruta;
        
        // Se o sensor lê de 95 a 300mm, nós convertemos essa escala para 45 a 250mm
        if (distanciaBruta >= 95 && distanciaBruta <= 300) {
            return map(distanciaBruta, 95, 300, 45, 250);
        }
    }

    return distanciaBruta;
}
