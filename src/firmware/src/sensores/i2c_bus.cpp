#include <Wire.h>
#include <Arduino.h>
#include "i2c_bus.h"
#include "../config/pinos.h"

void i2cInit() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);
    Serial.println("[I2C] Barramento inicializado");
}

void i2cScan() {
    Serial.println("[I2C] Iniciando scan...");
    int encontrados = 0;
    for (uint8_t addr = 0x01; addr <= 0x7F; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C]   Dispositivo encontrado: 0x%02X\n", addr);
            encontrados++;
        }
    }
    if (encontrados == 0) {
        Serial.println("[I2C]   Nenhum dispositivo encontrado — verificar cabos e pull-ups");
    } else {
        Serial.printf("[I2C] %d dispositivo(s) encontrado(s)\n", encontrados);
    }
}
