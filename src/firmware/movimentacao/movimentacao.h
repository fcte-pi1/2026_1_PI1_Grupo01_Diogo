#pragma once

/**
 * @file movimentacao.h
 * @brief API de movimentação de alto nível para o Micromouse.
 *
 * Fornece primitivas de deslocamento em grade (célula = 18 cm):
 *  - avancarCelula()    : avança uma célula em linha reta (open-loop temporal,
 *                         com correção de yaw via IMU — aguarda odometria na #73)
 *  - girarDireita90()   : gira 90° à direita via PID sobre o yaw integrado do IMU
 *  - girarEsquerda90()  : gira 90° à esquerda via PID sobre o yaw integrado do IMU
 *  - pararMovimentacao(): para os motores imediatamente
 *
 * Convenção de ângulo: positivo = esquerda (anti-horário visto de cima).
 * Ajustar IMU_GIRO_Z_SINAL em config/pinos.h se a convenção física for inversa.
 */

/**
 * Inicializa o módulo de movimentação.
 * Deve ser chamado após motoresInit() e imuInit().
 * @return true se os subsistemas estiverem prontos.
 */
bool movimentacaoInit();

/**
 * Avança uma célula do labirinto em linha reta (~18 cm).
 * Implementação atual: open-loop temporal com correção de yaw via IMU.
 * Será substituída por controle por odometria quando a issue #73 estiver pronta.
 */
void avancarCelula();

/**
 * Gira o robô 90° à direita no próprio eixo (pivot turn).
 * Controlado por PID sobre o yaw integrado do IMU.
 * Bloqueia até atingir o ângulo alvo ou estourar o timeout.
 */
void girarDireita90();

/**
 * Gira o robô 90° à esquerda no próprio eixo (pivot turn).
 * Controlado por PID sobre o yaw integrado do IMU.
 * Bloqueia até atingir o ângulo alvo ou estourar o timeout.
 */
void girarEsquerda90();

/**
 * Para os motores imediatamente (velocidade alvo = 0).
 */
void pararMovimentacao();
