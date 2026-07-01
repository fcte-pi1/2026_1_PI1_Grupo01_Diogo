#include <Arduino.h>
#include "../atuadores/motores.h"

unsigned long tempoInicial = 0;
bool comandoPararEnviado = false;

void setup() {
    Serial.begin(115200);
    if(!motoresInit()) {
        Serial.println("Falha ao iniciar motores!");
        while(true);
    }
    
    Serial.println("Acelerando... Vai rodar por 3 segundos.");
    motorSetVelocidade(MOTOR_ESQUERDO, 120);
    motorSetVelocidade(MOTOR_DIREITO, 120);
    
    tempoInicial = millis(); // Guarda o tempo de início
}

void loop() {
    // Obrigatório: atualiza a rampa de aceleração/desaceleração
    motoresAtualizar();

    // 1. Se passou 3 segundos, manda os motores pararem suavemente
    if (!comandoPararEnviado && (millis() - tempoInicial > 3000)) {
        Serial.println("3 segundos atingidos. Desacelerando...");
        motoresParar(); // Define o alvo de velocidade para 0
        comandoPararEnviado = true;
    }

    // 2. Se a rampa terminou de zerar a velocidade física, trava o ESP32
    if (comandoPararEnviado && 
        motorLerVelocidadeAtual(MOTOR_ESQUERDO) == 0 && 
        motorLerVelocidadeAtual(MOTOR_DIREITO) == 0) {
        
        Serial.println("Motores parados com sucesso. Teste encerrado.");
        while(true) {
            delay(1000); // Trava o ESP32 aqui para sempre
        }
    }
}