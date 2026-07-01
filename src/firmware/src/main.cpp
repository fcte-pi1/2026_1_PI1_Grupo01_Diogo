#include <Arduino.h>
#include "config/pinos.h"
#include "sensores/i2c_bus.h"
#include "sensores/tof.h"
#include "sensores/imu.h"
#include "atuadores/motores.h"

unsigned long ultimoControleMs = 0;
unsigned long tempoInicioMovimento = 0;
constexpr uint16_t INTERVALO_CONTROLE_MS = 20; 

// Parâmetros de Controle e Segurança
constexpr int16_t VELOCIDADE_BASE = 90; 
constexpr float KP = 4.5f;               
constexpr unsigned long TEMPO_MAXIMO_MOVIMENTO = 5000; // 5 segundos em milissegundos

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== INICIALIZANDO MICROMOUSE ===");

    i2cInit();

    if (!imuInit()) { Serial.println("[MAIN] ERRO: IMU."); while(true); }
    imuCalibrarOffsetZ(250); 
    imuZerarAnguloZ();

    if (!tofInit()) { Serial.println("[MAIN] ERRO: ToF."); while(true); }
    if (!motoresInit()) { Serial.println("[MAIN] ERRO: Motores."); while(true); }

    Serial.println("=== SISTEMA PRONTO: COLOQUE O ROBÔ NO CHÃO ===");
    delay(2000); 
    tempoInicioMovimento = millis(); // Inicia o cronômetro de segurança
}

// Variável global para acumular o erro (adicione no topo do arquivo se necessário)
static float somaErros = 0.0f; 

void loop() {
    imuAtualizar();     
    motoresAtualizar(); 

    unsigned long agora = millis();
    if (agora - ultimoControleMs >= INTERVALO_CONTROLE_MS) {
        ultimoControleMs = agora;

        // 1. Segurança por Tempo (5s)
        if (agora - tempoInicioMovimento >= TEMPO_MAXIMO_MOVIMENTO) {
            motoresParar();
            Serial.println("[PARADA] Tempo limite de 5s atingido.");
            while(true) { motoresAtualizar(); delay(10); }
        }

        // 2. Segurança Frontal (Filtrando o erro 8191)
        uint16_t distFrente = tofLerDistancia(1); 
        // Só aceita a leitura se for um valor real menor que 8000mm
        if (distFrente <= 150 && distFrente > 0) {
            motoresParar();
            Serial.printf("\n[FREIO] Parede vista a %dmm. Motores travados!\n", distFrente);
            while(true) { motoresAtualizar(); delay(10); }
        }

        // 3. Controle PI (Proporcional + Integral) de Linha Reta
        float anguloAtual = imuLerAnguloZ();
        float erro = anguloAtual - 0.0f; 

        // Acumula o erro ao longo do tempo (Integração)
        somaErros += erro;

        // Limitador da Integral (Anti-windup) para o motor não enlouquecer
        if (somaErros > 100.0f) somaErros = 100.0f;
        if (somaErros < -100.0f) somaErros = -100.0f;
        
        // Ganhos do Controle
        constexpr float KP_CORRIGIDO = 1.2f; 
        constexpr float KI_CORRIGIDO = 0.15f; // O "I" vai acumulando força contra o cabo

        // Fórmula do PI
        int16_t correcao = (int16_t)((erro * KP_CORRIGIDO) + (somaErros * KI_CORRIGIDO));

        int16_t velEsquerda = VELOCIDADE_BASE + correcao;
        int16_t velDireita  = VELOCIDADE_BASE - correcao;

        // Garante que o PWM não estoure os limites de 0 a 255
        if(velEsquerda > 255) velEsquerda = 255;
        if(velEsquerda < 0)   velEsquerda = 0;
        if(velDireita > 255)  velDireita = 255;
        if(velDireita < 0)    velDireita = 0;

        motorSetVelocidade(MOTOR_ESQUERDO, velEsquerda);
        motorSetVelocidade(MOTOR_DIREITO, velDireita);

        // Telemetria para análise
        Serial.printf("Ang:%.2f | Integral:%.1f | ToF:%dmm | Esq:%d | Dir:%d\n", 
                      anguloAtual, somaErros, distFrente, velEsquerda, velDireita);
    }
}