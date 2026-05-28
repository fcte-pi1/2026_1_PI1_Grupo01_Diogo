#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "imu.h"
#include "../config/pinos.h"

static Adafruit_MPU6050 mpu;
static float offsetGiroZ = 0.0f;

bool imuInit() {
    if (!mpu.begin(MPU6050_ADDR)) {
        Serial.println("[IMU] ERRO: MPU6050 não encontrado — verificar endereço e cabeamento");
        return false;
    }

    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.println("[IMU] MPU6050 inicializado (±250°/s, ±2g, filtro 21Hz)");
    return true;
}

float imuLerGiroZ() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    // Converter rad/s → °/s, aplicar sinal de montagem e subtrair offset de calibração
    return ((g.gyro.z * 180.0f / M_PI) - offsetGiroZ) * IMU_GIRO_Z_SINAL;
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
