#include <Arduino.h>

// --- Dependências de Hardware e Módulos ---
#include "config/pinos.h"
#include "sensores/i2c_bus.h"
#include "sensores/encoders.h"
#include "sensores/imu.h"
#include "sensores/tof.h"
#include "atuadores/motores.h"

// --- Máquina de Estados Finitos (FSM) ---
enum EstadoRobo {
    PENSANDO,
    ANDANDO_RETO,
    VIRANDO_EIXO,
    FIM_DE_PISTA
};

EstadoRobo estadoAtual = PENSANDO; 

// --- API de Movimentação ---

// Aplica PWM igual em ambos os motores (Base para futura integração do PID)
void andarParaFrente(int velocidade) {
    motorSetVelocidade(0, velocidade);
    motorSetVelocidade(1, velocidade);
}

// Rotação diferencial (Tank Turn) para zerar o raio de curva
void girarNoEixo(int velocidade) {
    motorSetVelocidade(0, -velocidade); 
    motorSetVelocidade(1, velocidade);
}

// Zera as velocidades alvo e aciona a rampa de desaceleração
void pararTudo() {
    motoresParar(); 
}

// --- Inicialização do Sistema ---
void setup() {
    Serial.begin(115200);
    Serial.println("[Init] Boot do sistema iniciado.");

    // Configuração de barramento e interrupções de hardware (Odometria)
    i2cInit(); 
    encodersInit();

    // Calibra o offset do giroscópio
    imuCalibrarOffsetZ(int amostras = 400);

    // Inicialização de sensores I2C (IMU MPU6050 e ToF VL53L0X)
    imuInit();
    tofInit();

    // Configuração dos geradores de PWM e driver TB6612FNG
    motoresInit();

    // Estabilização de transientes elétricos antes do loop de controle
    delay(1000);
    Serial.println("[Init] FSM pronta para execução.");
}

// --- Loop Principal (Controle e Navegação) ---
void loop() {
    // Atualização da rampa de PWM (Execução não-bloqueante obrigatória)
    motoresAtualizar();
    imuAtualizar();

    // Controle de transição da FSM
    switch (estadoAtual) {
        
        case PENSANDO:
            // Algoritmo Flood Fill: Leitura de mapa e decisão de rota.
           
            break;

        case ANDANDO_RETO:
            andarParaFrente(150);

            // Condição de parada: Odometria atingiu 1 célula (ex: 500 pulsos) 
            // OU sistema anti-colisão detectou obstáculo a < 50mm.
            if (encoderLerEsquerdo() >= 500 || tofLerDistancia(0) < 50) {
                pararTudo(); 
                estadoAtual = PENSANDO; 
            }
            break;

        case VIRANDO_EIXO:
            girarNoEixo(150);

           if (abs(imuLerAnguloZ()) >= 90.0f) { 
                pararTudo(); 
                estadoAtual = PENSANDO; 
            }
            break;

        case FIM_DE_PISTA:
            pararTudo();
            // Objetivo alcançado (Centro do labirinto). Retém o robô em idle.
            break;
    }
}