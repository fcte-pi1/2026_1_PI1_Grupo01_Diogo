#include <Arduino.h>
#include "../sensores/i2c_bus.h" // Puxa o seu inicializador e scan
#include "../sensores/imu.h"

unsigned long ultimoPrint = 0;

void setup() {
    Serial.begin(115200);
    
    // 1. Inicializa o barramento usando os seus pinos do pinos.h
    i2cInit(); 
    
    // 2. Roda o scan para termos certeza absoluta de que o MPU6050 respondeu
    i2cScan(); 
    
    // 3. Inicializa o sensor físico
    if(!imuInit()) {
        Serial.println("Falha catastrófica ao iniciar o MPU6050!");
        while(true);
    }
    
    // Calibração inicial (O robô DEVE estar imóvel aqui)
    imuCalibrarOffsetZ(200); 
    imuZerarAnguloZ();
    
    Serial.println("\n--- TESTE DE ORIENTAÇÃO INICIADO ---");
    Serial.println("Gire o robô na horizontal (Eixo Z)...");
}

void loop() {
    // ESSENCIAL: Atualiza a integração do ângulo continuamente
    imuAtualizar();

    // Imprime o ângulo a cada 100ms para não inundar o terminal
    if (millis() - ultimoPrint >= 100) {
        ultimoPrint = millis();
        
        float angulo = imuLerAnguloZ();
        float velocidade = imuLerGiroZ();
        
        Serial.print("V_Giro_Z: ");
        Serial.print(velocidade, 2);
        Serial.print(" °/s\t|\tANGULO_Z: ");
        Serial.print(angulo, 2);
        Serial.println(" °");
    }
}