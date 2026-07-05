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

    // Sensor 2 (direita): esta unidade tem um OFFSET CONSTANTE de ~+50 mm em toda
    // a faixa (lê 50 mm a mais que o real, de forma proporcional). Corrige -50 mm.
    // NOTA 1: map(95,300,45,250) é matematicamente IDÊNTICO a (distanciaBruta - 50).
    // NOTA 2: a correção só é aplicada em 95..300 mm, que cobre a faixa de operação
    //         (parede lateral e o limiar de ~140 mm). Fora dela (muito perto / muito
    //         longe) a decisão de parede não muda, então segue sem corrigir.
    if (i == 2) {
        if (distanciaBruta > 2000) return distanciaBruta;

        if (distanciaBruta >= 95 && distanciaBruta <= 300) {
            return map(distanciaBruta, 95, 300, 45, 250);
        }
    }

    return distanciaBruta;
}
