#include <Arduino.h>
#include "encoders.h"
#include "../config/pinos.h" // Garanta que os pinos dos encoders estão definidos aqui

// 'volatile' avisa o compilador que essas variáveis podem mudar a qualquer milissegundo
// por fatores externos (o hardware), evitando que ele otimize e apague elas da memória.
volatile int32_t pulsosEsquerdo = 0;
volatile int32_t pulsosDireito = 0;

// IRAM_ATTR força essa função a rodar na memória RAM mais rápida do ESP32.
// Interrupções não podem esperar o processador buscar código na memória Flash lenta.
void IRAM_ATTR isrEncoderEsquerdo() {
    pulsosEsquerdo++;
}

void IRAM_ATTR isrEncoderDireito() {
    pulsosDireito++;
}

bool encodersInit() {
    // Configura os pinos como entrada com resistor interno (evita flutuação de sinal)
    pinMode(ENCODER_ESQ_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DIR_PIN, INPUT_PULLUP);

    // Atrela a interrupção ao pino. 'RISING' significa que vai contar
    // toda vez que o sinal elétrico subir de LOW para HIGH (borda de subida).
    attachInterrupt(digitalPinToInterrupt(ENCODER_ESQ_PIN), isrEncoderEsquerdo, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DIR_PIN), isrEncoderDireito, RISING);

    Serial.println("[Encoders] Interrupções de hardware inicializadas");
    return true;
}

int32_t encoderLerEsquerdo() {
    return pulsosEsquerdo;
}

int32_t encoderLerDireito() {
    return pulsosDireito;
}

void encodersZerar() {
    // Desliga as interrupções temporariamente para evitar que
    // a variável mude exatamente no milissegundo em que estamos zerando ela.
    noInterrupts();
    pulsosEsquerdo = 0;
    pulsosDireito = 0;
    interrupts();
}