#include <Arduino.h>
#include <ESP32Encoder.h>
#include "encoders.h"
#include "../config/pinos.h"

const float PPR = 570.0; 
const float CIRCUNFERENCIA_CM = 14.06;  // calibrado: 50 cm reais = ~2027 pulsos 

ESP32Encoder encoderEsquerdo;
ESP32Encoder encoderDireito;

bool encodersInit() {
    // Pull-ups dos encoders:
    //  - 32/33 (ESQ): GPIOs normais -> usam o pull-up INTERNO do ESP32.
    //  - 34/35 (DIR): input-only, NAO tem pull-up interno -> pull-up EXTERNO no HW.
    // Por isso 34/35 ficam como INPUT (INPUT_PULLUP ali seria no-op).
    pinMode(ENCODER_ESQ_PINA, INPUT_PULLUP);
    pinMode(ENCODER_ESQ_PINB, INPUT_PULLUP);
    pinMode(ENCODER_DIR_PINA, INPUT);
    pinMode(ENCODER_DIR_PINB, INPUT);

    // Politica GLOBAL do ESP32Encoder. 'up' e o correto no cenario misto acima:
    // liga o interno onde da (32/33) e e inocuo em 34/35. So troque para
    // puType::none se 32/33 TAMBEM ganharem pull-up externo.
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