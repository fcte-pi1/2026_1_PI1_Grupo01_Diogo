#pragma once
#include <stdint.h>

bool     tofInit();                // inicializa NUM_TOF sensores com endereços únicos
uint16_t tofLerDistancia(int i);   // distância do sensor i em mm (0 .. NUM_TOF-1)
