#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Leitura da bateria via INA219.
//
// O módulo converte a tensão do pack 2S (~6.0V a ~8.4V) para percentual e
// expõe uma API pequena para a telemetria usar sem conhecer detalhes do sensor.
// -----------------------------------------------------------------------------

bool energiaInit();
uint8_t energiaLerPct();