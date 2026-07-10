#include "pid.h"

PID::PID(float kp, float ki, float kd)
    : _kp(kp), _ki(ki), _kd(kd),
      _integral(0.0f), _erroAnterior(0.0f), _temErroAnterior(false),
      _saidaMin(-255.0f), _saidaMax(255.0f), _limiteIntegral(1000.0f),
      _p(0.0f), _i(0.0f), _d(0.0f) {}

void PID::definirGanhos(float kp, float ki, float kd) {
    _kp = kp;
    _ki = ki;
    _kd = kd;
}

void PID::definirLimiteSaida(float minimo, float maximo) {
    _saidaMin = minimo;
    _saidaMax = maximo;
}

void PID::definirLimiteIntegral(float limite) {
    _limiteIntegral = fabsf(limite);
}

void PID::resetar() {
    _integral = 0.0f;
    _erroAnterior = 0.0f;
    _temErroAnterior = false;
    _p = _i = _d = 0.0f;
}

float PID::calcular(float erro, float dt) {
    if (dt <= 0.0f) dt = 1e-3f;  // proteção contra divisão por zero

    // --- Proporcional ---
    _p = _kp * erro;

    // --- Derivativo (sobre o erro) ---
    if (_temErroAnterior) {
        _d = _kd * ((erro - _erroAnterior) / dt);
    } else {
        _d = 0.0f;                 // sem derivada no primeiro ciclo (evita "chute")
        _temErroAnterior = true;
    }
    _erroAnterior = erro;

    // --- Integral com ANTI-WINDUP por integração condicional ---
    // Tentamos integrar; se a saída ficaria saturada, só aceitamos a nova
    // integral se ela EMPURRA para FORA da saturação. Assim o termo I não
    // "enche" durante a fase rápida (saída no teto) e só ganha autoridade
    // no finalzinho, quando P sozinho é fraco demais para vencer o atrito.
    float integralTent = _integral + erro * dt;
    if (integralTent >  _limiteIntegral) integralTent =  _limiteIntegral;
    if (integralTent < -_limiteIntegral) integralTent = -_limiteIntegral;

    float saidaTent = _p + (_ki * integralTent) + _d;

    bool aceitarIntegral;
    if (saidaTent > _saidaMax) {
        aceitarIntegral = (erro < 0.0f);   // saturado no topo: só integra se reduz a saída
    } else if (saidaTent < _saidaMin) {
        aceitarIntegral = (erro > 0.0f);   // saturado embaixo: só integra se aumenta a saída
    } else {
        aceitarIntegral = true;            // não saturado: integra normalmente
    }
    if (aceitarIntegral) {
        _integral = integralTent;
    }
    _i = _ki * _integral;

    // --- Saída saturada ---
    float saida = _p + _i + _d;
    if (saida > _saidaMax) saida = _saidaMax;
    if (saida < _saidaMin) saida = _saidaMin;
    return saida;
}

float PID::calcular(float setpoint, float medida, float dt) {
    return calcular(setpoint - medida, dt);
}