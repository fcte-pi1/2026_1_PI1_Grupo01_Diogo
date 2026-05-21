#pragma once

bool  imuInit();
float imuLerGiroZ();                          // velocidade angular eixo Z em °/s
void  imuCalibrarOffsetZ(int amostras = 1000); // deve ser chamada com robô parado
