#pragma once

#include <stdint.h>

enum MotorId : uint8_t {
    MOTOR_ESQUERDO = 0,
    MOTOR_DIREITO  = 1,
    NUM_MOTORES    = 2
};

enum SentidoMotor : int8_t {
    MOTOR_RE       = -1,
    MOTOR_PARADO   = 0,
    MOTOR_FRENTE   = 1
};

bool motoresInit();
void motoresAtualizar();
void motoresHabilitar(bool habilitar);

// -----------------------------------------------------------------------------
// MODO LEGADO (por motor) — controle individual com RAMPA (slew-rate).
// Usado por teste_motores.cpp e, por enquanto, por navegacao.cpp.
// Chamar qualquer um destes coloca o driver de volta no modo por-motor.
// -----------------------------------------------------------------------------
void motorSetVelocidade(MotorId motor, int16_t velocidade);
void motorSetComando(MotorId motor, SentidoMotor sentido, uint8_t pwm);
void motorParar(MotorId motor);

// -----------------------------------------------------------------------------
// MODO CRUZEIRO+CORRECAO — para controle de rumo/curva.
//   - A BASE (cruzeiro) é rampeada suavemente (evita picos de corrente/derrapagem).
//   - A CORRECAO (diferencial) é aplicada IMEDIATAMENTE, sem rampa, para o PID
//     ter autoridade instantânea.
//   Saída final:  esquerdo = base - correcao ; direito = base + correcao.
// Chamar qualquer um destes ativa o modo cruzeiro.
// -----------------------------------------------------------------------------
void motoresSetCruzeiro(int16_t base);       // velocidade de avanço comum (rampeada)
void motoresSetCorrecao(int16_t correcao);   // diferencial imediato (PID)

// Para ambos os modos: zera cruzeiro, correcao e alvos por-motor.
void motoresParar();

// Freio ATIVO (short-brake): curto-circuita os motores (as duas entradas do
// TB6612 em HIGH) pra parar seco, matando o rolamento livre. Persiste até o
// próximo comando de movimento. Use de preferência JÁ EM BAIXA velocidade.
void motoresFrear();

int16_t motorLerVelocidadeAtual(MotorId motor);  // saída aplicada AGORA (inclui rampa)
int16_t motorLerVelocidadeAlvo(MotorId motor);
