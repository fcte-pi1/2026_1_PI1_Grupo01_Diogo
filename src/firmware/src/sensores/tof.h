#pragma once
#include <stdint.h>

bool     tofInit();                // inicializa NUM_TOF sensores com endereços únicos
uint16_t tofLerDistancia(int i);        // distância do sensor i em mm (com correção)
uint16_t tofLerDistanciaBruta(int i);   // valor CRU do sensor, sem correção (diagnóstico)
