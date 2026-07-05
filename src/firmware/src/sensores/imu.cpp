#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "imu.h"
#include "../config/pinos.h"

static Adafruit_MPU6050 mpu;
static float offsetGiroZ = 0.0f;
static float anguloAcumuladoZ = 0.0f;
static unsigned long tempoAnteriorIMU = 0;

bool imuInit() {
    if (!mpu.begin(MPU6050_ADDR)) {
        Serial.println("[IMU] ERRO: MPU6050 não encontrado — verificar endereço e cabeamento");
        return false;
    }

    // ±500°/s: o giro de curva satura ±250°/s (velGiro grudava em 249 em toda
    // curva), subcontando o ângulo. ±500 dá folga pra medir a rotação real.
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    tempoAnteriorIMU = millis();
    Serial.println("[IMU] MPU6050 inicializado (±500°/s, ±2g, filtro 21Hz)");
    return true;
}

float imuLerGiroZ() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    // Converter rad/s → °/s e subtrair offset de calibração
    return (g.gyro.z * 180.0f / M_PI) - offsetGiroZ;
}

void imuCalibrarOffsetZ(int amostras) {
    Serial.printf("[IMU] Calibrando offset Z com %d amostras (mantenha o robô parado)...\n", amostras);
    double soma = 0.0;
    sensors_event_t a, g, temp;
    for (int i = 0; i < amostras; i++) {
        mpu.getEvent(&a, &g, &temp);
        soma += g.gyro.z * 180.0 / M_PI;
        delay(2);
    }
    offsetGiroZ = (float)(soma / amostras);
    Serial.printf("[IMU] Offset Z calibrado: %.4f °/s\n", offsetGiroZ);
}


// --- Funções de Navegação e Odometria Angular ---
void imuAtualizar() {
    unsigned long tempoAtual = millis();
    float dt = (tempoAtual - tempoAnteriorIMU) / 1000.0f;
    tempoAnteriorIMU = tempoAtual;

    if (dt > 0.2f || dt <= 0.0f) return;

    float velAngularZ = imuLerGiroZ();

    anguloAcumuladoZ += (velAngularZ * dt);
}

float imuLerAnguloZ() {
    return anguloAcumuladoZ;
}

void imuZerarAnguloZ() {
    anguloAcumuladoZ = 0.0f;
    tempoAnteriorIMU = millis(); 
}
