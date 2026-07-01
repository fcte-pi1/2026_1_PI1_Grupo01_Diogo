#pragma once

bool  imuInit();
float imuLerGiroZ();                          // velocidade angular eixo Z em °/s
void  imuCalibrarOffsetZ(int amostras); // deve ser chamada com robô parado
void imuAtualizar();
float imuLerAnguloZ();
void imuZerarAnguloZ();
