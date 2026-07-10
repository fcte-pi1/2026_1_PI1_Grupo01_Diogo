#include "sensores/energia.h"

#include <Wire.h>
#include <Adafruit_INA219.h>

namespace {

Adafruit_INA219 ina219;
bool energiaPronta = false;

constexpr float TENSAO_MIN_PACK_2S = 6.0f;
constexpr float TENSAO_MAX_PACK_2S = 8.4f;

uint8_t tensaoParaPct(float tensao) {
    if (tensao <= TENSAO_MIN_PACK_2S) return 0;
    if (tensao >= TENSAO_MAX_PACK_2S) return 100;

    const float frac = (tensao - TENSAO_MIN_PACK_2S) /
                       (TENSAO_MAX_PACK_2S - TENSAO_MIN_PACK_2S);
    return (uint8_t)constrain((int)roundf(frac * 100.0f), 0, 100);
}

float lerTensaoPackV() {
    const float busV = ina219.getBusVoltage_V();
    const float shuntV = ina219.getShuntVoltage_mV() / 1000.0f;
    return busV + shuntV;
}

}  // namespace

bool energiaInit() {
    energiaPronta = ina219.begin();
    if (!energiaPronta) {
        Serial.println("[ENERGIA] INA219 nao encontrado; percentual de bateria ficara em reserva.");
        return false;
    }

    Serial.println("[ENERGIA] INA219 pronto.");
    return true;
}

uint8_t energiaLerPct() {
    if (!energiaPronta) return 100;

    const float tensao = lerTensaoPackV();
    return tensaoParaPct(tensao);
}