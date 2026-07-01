#include <Arduino.h>
#include <ESP32Encoder.h>
#include "encoders.h"
#include "../config/pinos.h"

const float PPR = 570.0; 
const float CIRCUNFERENCIA_CM = 14.13; 

ESP32Encoder encoderEsquerdo;
ESP32Encoder encoderDireito;

bool encodersInit() {
    pinMode(ENCODER_ESQ_PINA, INPUT_PULLUP);
    pinMode(ENCODER_ESQ_PINB, INPUT_PULLUP);
    pinMode(ENCODER_DIR_PINA, INPUT_PULLUP);
    pinMode(ENCODER_DIR_PINB, INPUT_PULLUP);

    ESP32Encoder::useInternalWeakPullResistors = puType::up;

    encoderEsquerdo.attachFullQuad(ENCODER_ESQ_PINA, ENCODER_ESQ_PINB);
    encoderDireito.attachFullQuad(ENCODER_DIR_PINA, ENCODER_DIR_PINB);

    encoderEsquerdo.clearCount();
    encoderDireito.clearCount();

    return true;
}

int32_t encoderLerEsquerdo() {
    return (int32_t)encoderEsquerdo.getCount();
}

int32_t encoderLerDireito() {
    return (int32_t)encoderDireito.getCount();
}

void encodersZerar() {
    encoderEsquerdo.clearCount();
    encoderDireito.clearCount();
}

float encoderDistanciaEsquerdaCm() {
    int32_t pulsos = encoderLerEsquerdo();
    return (pulsos / PPR) * CIRCUNFERENCIA_CM;
}

float encoderDistanciaDireitaCm() {
    int32_t pulsos = encoderLerDireito();
    return (pulsos / PPR) * CIRCUNFERENCIA_CM;
}