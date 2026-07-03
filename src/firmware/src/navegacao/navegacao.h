#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Camada de navegação: primitivas de movimento controladas por PID.
//
// - Reta: PID de manutenção de rumo (heading) usando o ângulo Z da IMU,
//   com opção de centralização entre paredes via ToF laterais.
//   O deslocamento de uma célula é medido pelos encoders.
// - Curvas: PID sobre o erro de ângulo (IMU) até 90°/180° com tolerância.
//
// As primitivas são BLOQUEANTES e chamam imuAtualizar()/motoresAtualizar()
// a cada iteração, então não precisam ser chamadas dentro do loop principal.
//
// Pré-requisito: imuInit(), tofInit() e motoresInit() já devem ter rodado.
// navInit() cuida de encodersInit().
// -----------------------------------------------------------------------------

bool navInit();

// --- Leitura de paredes na ORIENTAÇÃO ATUAL do robô (relativo ao robô) ---
bool navParedeFrente();
bool navParedeEsquerda();
bool navParedeDireita();

// --- Primitivas de movimento (bloqueantes) ---
void navAndarUmaCelula();   // avança 1 célula mantendo o rumo
void navGirarDireita();     // gira -90° (horário)
void navGirarEsquerda();    // gira +90° (anti-horário)
void navGirarMeiaVolta();   // gira 180°
void navParar();            // para os motores

// Reajusta o rumo de referência para o valor atual da IMU (ex.: no início).
void navZerarRumo();
