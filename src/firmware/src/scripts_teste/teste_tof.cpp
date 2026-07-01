#include <Arduino.h>
#include "../sensores/i2c_bus.h" // Garante o uso dos seus pinos I2C
#include "../sensores/tof.h"
#include "../config/pinos.h"

unsigned long ultimoPrint = 0;

void setup() {
    Serial.begin(115200);
    
    // 1. Inicializa o barramento com seus pinos do pinos.h
    i2cInit(); 
    
    // 2. Inicializa a sequência de boot dos sensores ToF
    if (!tofInit()) {
        Serial.println("Falha catastrófica ao inicializar os sensores ToF!");
        while (true);
    }
    
    Serial.println("\n--- TESTE DE SENSORES TOF INICIADO ---");
    Serial.println("Aproxime as maos dos sensores para ver a distancia mudando...");
}

void loop() {
    // Imprime as leituras a cada 100ms para ficar scannear bem na tela
    if (millis() - ultimoPrint >= 100) {
        ultimoPrint = millis();
        
        Serial.print("ToF ->  ");
        for (int i = 0; i < NUM_TOF; i++) {
            uint16_t dist = tofLerDistancia(i);
            
            Serial.print("S[");
            Serial.print(i);
            Serial.print("]: ");
            if (dist > 2000) { 
                Serial.print("LIVRE"); // Valor muito alto significa sem obstáculo próximo
            } else {
                Serial.print(dist);
                Serial.print("mm");
            }
            Serial.print("\t|\t");
        }
        Serial.println();
    }
}