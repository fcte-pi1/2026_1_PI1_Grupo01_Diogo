#pragma once

// --- Barramento I2C ---
#define I2C_SDA_PIN   21
#define I2C_SCL_PIN   22
#define I2C_FREQ_HZ   400000  // 400 kHz; reduzir para 100000 se houver instabilidade

// --- IMU MPU6050 ---
#define MPU6050_ADDR  0x68  // 0x69 se AD0 ligado a VCC

// --- Sensores ToF VL53L0X ---
// Confirmado pelo esquemático: 3 sensores (Frontal, Esquerda, Direita)
// ⚠️ Confirmar GPIOs reais dos pinos XSHUT_F, XSHUT_E, XSHUT_D no esquemático antes de gravar
#define NUM_TOF       3

// Ordem: 0 = Frontal, 1 = Esquerda, 2 = Direita
// Pinos XSHUT devem ser saída — NÃO usar GPIO 34–39 (input-only)
constexpr int     TOF_XSHUT_PINS[NUM_TOF] = { 25, 26, 32 };
constexpr uint8_t TOF_ADDRESSES[NUM_TOF]  = { 0x30, 0x31, 0x32 };

// --- Driver de Motores TB6612FNG ---
// Ajustar estes GPIOs caso o esquemático final use outro roteamento.
// Evitar GPIO 34-39: são somente entrada e não servem para PWM/direção.
#define MOTOR_AIN1_PIN  16
#define MOTOR_AIN2_PIN  17
#define MOTOR_PWMA_PIN  18

#define MOTOR_BIN1_PIN  19
#define MOTOR_BIN2_PIN  23
#define MOTOR_PWMB_PIN  27

#define MOTOR_STBY_PIN  33

// --- Encoders ---
// ⚠️ Atenção: Pinos de interrupção precisam suportar INPUT_PULLUP interno.
// Evite os pinos 34 a 39, pois eles não possuem resistor de pull-up interno.
// Ajuste esses números para bater com o esquemático/solda real do robô!
#define ENCODER_ESQ_PIN  14
#define ENCODER_DIR_PIN  15
#define NUM_MOTORES       2 // Adicionado para garantir a compilação do motores.cpp
