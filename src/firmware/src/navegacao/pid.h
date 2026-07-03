#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Controlador PID genérico e reutilizável.
//
// Usado tanto para manter a linha reta (erro = ângulo alvo - ângulo atual)
// quanto para girar 90°/180° (erro = ângulo alvo - ângulo atual, saída = giro).
//
// Recursos:
//   - Anti-windup no termo integral (limiteIntegral).
//   - Saturação da saída (limiteSaida).
//   - Derivada calculada sobre o erro, com proteção para o primeiro ciclo.
// -----------------------------------------------------------------------------
class PID {
public:
    PID(float kp = 0.0f, float ki = 0.0f, float kd = 0.0f);

    void definirGanhos(float kp, float ki, float kd);
    void definirLimiteSaida(float minimo, float maximo);   // satura a saída
    void definirLimiteIntegral(float limite);              // anti-windup (|I| <= limite)
    void resetar();                                        // zera histórico (chamar antes de cada movimento)

    // Calcula a saída de controle a partir do erro já pronto.
    float calcular(float erro, float dt);

    // Conveniência: calcula o erro internamente (setpoint - medida).
    float calcular(float setpoint, float medida, float dt);

    // Acesso aos termos para depuração/telemetria.
    float termoP() const { return _p; }
    float termoI() const { return _i; }
    float termoD() const { return _d; }

private:
    float _kp, _ki, _kd;

    float _integral;
    float _erroAnterior;
    bool  _temErroAnterior;

    float _saidaMin, _saidaMax;
    float _limiteIntegral;

    // Últimos termos (para depuração).
    float _p, _i, _d;
};
