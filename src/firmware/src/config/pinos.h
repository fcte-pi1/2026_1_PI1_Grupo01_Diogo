#pragma once

// --- Barramento I2C ---
#define I2C_SDA_PIN   21
#define I2C_SCL_PIN   22
#define I2C_FREQ_HZ   400000  

// --- IMU MPU6050 ---
#define MPU6050_ADDR  0x68  

// --- Sensores ToF VL53L0X ---
#define NUM_TOF       3

constexpr int     TOF_XSHUT_PINS[NUM_TOF] = { 26, 4, 23 };
constexpr uint8_t TOF_ADDRESSES[NUM_TOF]  = { 0x30, 0x31, 0x32 };

// --- Driver de Motores TB6612FNG ---
// Canal A (Motor 1)
#define MOTOR_AIN1_PIN  13  
#define MOTOR_AIN2_PIN  25
#define MOTOR_PWMA_PIN  18

// Canal B (Motor 2)
#define MOTOR_BIN1_PIN  14
#define MOTOR_BIN2_PIN  27
#define MOTOR_PWMB_PIN  19

// Ativação da Ponte H
#define MOTOR_STBY_PIN  5

// --- Encoders 
#define ENCODER_ESQ_PINA  32
#define ENCODER_ESQ_PINB  33

#define ENCODER_DIR_PINA  34
#define ENCODER_DIR_PINB  35