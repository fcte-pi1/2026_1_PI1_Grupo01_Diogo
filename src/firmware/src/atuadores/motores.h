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

void motorSetVelocidade(MotorId motor, int16_t velocidade);
void motorSetComando(MotorId motor, SentidoMotor sentido, uint8_t pwm);
void motorParar(MotorId motor);
void motoresParar();

int16_t motorLerVelocidadeAtual(MotorId motor);
int16_t motorLerVelocidadeAlvo(MotorId motor);
