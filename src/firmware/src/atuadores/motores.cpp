#include <Arduino.h>
#include "motores.h"
#include "../config/pinos.h"

namespace {
constexpr uint32_t PWM_FREQ_HZ = 20000;
constexpr uint8_t PWM_RES_BITS = 8;
constexpr uint8_t PWM_MAX = 255;
constexpr uint8_t PWM_CANAL_ESQUERDO = 0;
constexpr uint8_t PWM_CANAL_DIREITO = 1;

constexpr uint8_t RAMPA_PASSO_PWM = 5;
constexpr uint16_t RAMPA_INTERVALO_MS = 10;

struct MotorConfig {
    uint8_t in1;
    uint8_t in2;
    uint8_t pwm;
    uint8_t canalPwm;
};

struct MotorEstado {
    int16_t velocidadeAtual;
    int16_t velocidadeAlvo;
};

const MotorConfig configs[NUM_MOTORES] = {
    { MOTOR_AIN1_PIN, MOTOR_AIN2_PIN, MOTOR_PWMA_PIN, PWM_CANAL_ESQUERDO },
    { MOTOR_BIN1_PIN, MOTOR_BIN2_PIN, MOTOR_PWMB_PIN, PWM_CANAL_DIREITO }
};

MotorEstado estados[NUM_MOTORES] = {
    { 0, 0 },
    { 0, 0 }
};

bool driverHabilitado = false;
uint32_t ultimoPassoRampaMs = 0;

int16_t limitarVelocidade(int16_t velocidade) {
    if (velocidade > PWM_MAX) return PWM_MAX;
    if (velocidade < -PWM_MAX) return -PWM_MAX;
    return velocidade;
}

bool motorValido(MotorId motor) {
    return motor < NUM_MOTORES;
}

bool pwmAnexar(const MotorConfig& cfg) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    return ledcAttach(cfg.pwm, PWM_FREQ_HZ, PWM_RES_BITS);
#else
    ledcSetup(cfg.canalPwm, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttachPin(cfg.pwm, cfg.canalPwm);
    return true;
#endif
}

void pwmEscrever(const MotorConfig& cfg, uint8_t valor) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(cfg.pwm, valor);
#else
    ledcWrite(cfg.canalPwm, valor);
#endif
}

void aplicarSaida(MotorId motor, int16_t velocidade) {
    const MotorConfig& cfg = configs[motor];
    const uint8_t pwm = abs(velocidade);

    if (!driverHabilitado || velocidade == 0) {
        digitalWrite(cfg.in1, LOW);
        digitalWrite(cfg.in2, LOW);
        pwmEscrever(cfg, 0);
        return;
    }

    if (velocidade > 0) {
        digitalWrite(cfg.in1, HIGH);
        digitalWrite(cfg.in2, LOW);
    } else {
        digitalWrite(cfg.in1, LOW);
        digitalWrite(cfg.in2, HIGH);
    }

    pwmEscrever(cfg, pwm);
}

int16_t aproximar(int16_t atual, int16_t alvo) {
    if (atual < alvo) {
        atual += RAMPA_PASSO_PWM;
        return atual > alvo ? alvo : atual;
    }

    if (atual > alvo) {
        atual -= RAMPA_PASSO_PWM;
        return atual < alvo ? alvo : atual;
    }

    return atual;
}
}

bool motoresInit() {
    pinMode(MOTOR_STBY_PIN, OUTPUT);
    digitalWrite(MOTOR_STBY_PIN, LOW);

    for (uint8_t i = 0; i < NUM_MOTORES; i++) {
        pinMode(configs[i].in1, OUTPUT);
        pinMode(configs[i].in2, OUTPUT);

        if (!pwmAnexar(configs[i])) {
            Serial.printf("[Motores] ERRO: falha ao configurar PWM do motor %u\n", i);
            return false;
        }

        estados[i].velocidadeAtual = 0;
        estados[i].velocidadeAlvo = 0;
        aplicarSaida((MotorId)i, 0);
    }

    motoresHabilitar(true);
    Serial.println("[Motores] TB6612FNG inicializado (PWM 20kHz, 8 bits)");
    return true;
}

void motoresAtualizar() {
    const uint32_t agora = millis();
    if (agora - ultimoPassoRampaMs < RAMPA_INTERVALO_MS) return;
    ultimoPassoRampaMs = agora;

    for (uint8_t i = 0; i < NUM_MOTORES; i++) {
        estados[i].velocidadeAtual = aproximar(estados[i].velocidadeAtual,
                                               estados[i].velocidadeAlvo);
        aplicarSaida((MotorId)i, estados[i].velocidadeAtual);
    }
}

void motoresHabilitar(bool habilitar) {
    driverHabilitado = habilitar;
    digitalWrite(MOTOR_STBY_PIN, habilitar ? HIGH : LOW);

    if (!habilitar) {
        for (uint8_t i = 0; i < NUM_MOTORES; i++) {
            estados[i].velocidadeAtual = 0;
            aplicarSaida((MotorId)i, 0);
        }
    }
}

void motorSetVelocidade(MotorId motor, int16_t velocidade) {
    if (!motorValido(motor)) return;
    estados[motor].velocidadeAlvo = limitarVelocidade(velocidade);
}

void motorSetComando(MotorId motor, SentidoMotor sentido, uint8_t pwm) {
    if (!motorValido(motor)) return;

    if (sentido == MOTOR_PARADO || pwm == 0) {
        motorSetVelocidade(motor, 0);
        return;
    }

    const int16_t velocidade = (sentido == MOTOR_FRENTE) ? pwm : -((int16_t)pwm);
    motorSetVelocidade(motor, velocidade);
}

void motorParar(MotorId motor) {
    motorSetVelocidade(motor, 0);
}

void motoresParar() {
    for (uint8_t i = 0; i < NUM_MOTORES; i++) {
        motorParar((MotorId)i);
    }
}

int16_t motorLerVelocidadeAtual(MotorId motor) {
    if (!motorValido(motor)) return 0;
    return estados[motor].velocidadeAtual;
}

int16_t motorLerVelocidadeAlvo(MotorId motor) {
    if (!motorValido(motor)) return 0;
    return estados[motor].velocidadeAlvo;
}
