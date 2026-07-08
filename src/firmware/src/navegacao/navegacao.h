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
void navAndarUmaCelula(bool re = false);  // avança 1 célula (re=true: anda de RÉ, p/ beco/180)
bool navEsquadrar();        // anda até TOCAR a parede da frente (esquadra); true se encostou
void navGirarDireita();     // gira -90° (horário)
void navGirarEsquerda();    // gira +90° (anti-horário)
void navGirarMeiaVolta();   // gira 180°
void navParar();            // para os motores

// Reajusta o rumo de referência para o valor atual da IMU (ex.: no início).
void navZerarRumo();

// Ajusta o viés do giro (graus que o giro para ANTES de 90°). Calibração — a
// bateria cheia desliza mais, então às vezes precisa de mais viés.
void navDefinirViesCurva(float graus);

// Calibração ao vivo da CENTRALIZAÇÃO (cascata posição->rumo). Contra a serpenteada:
//   Kp       = quão forte corrige por unidade de descentralização (menor=suave/lento).
//   ViesMax  = teto do ângulo de nariz usado p/ transladar (menor=excursões menores).
//   Deadband = zona "perto o bastante" onde para de corrigir (maior=mata o vai-e-vem).
void navDefinirCentroKp(float v);
void navDefinirCentroViesMax(float v);
void navDefinirCentroDeadband(float v);
