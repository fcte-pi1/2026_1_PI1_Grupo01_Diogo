#pragma once

#include <Arduino.h>

// Configura os pinos e as interrupções de hardware dos encoders
bool encodersInit();

// Retorna a contagem acumulada de pulsos
int32_t encoderLerEsquerdo();
int32_t encoderLerDireito();

// Zera os contadores (necessário antes de iniciar novos movimentos no PID)
void encodersZerar();

// Retorna a distância percorrida em centímetros
float encoderDistanciaEsquerdaCm();
float encoderDistanciaDireitaCm();