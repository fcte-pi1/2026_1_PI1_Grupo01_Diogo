#pragma once

#include <Arduino.h>
#include "navegacao/flood_fill.h"   // enum Direcao

// -----------------------------------------------------------------------------
// Telemetria por WebSocket — o robô é CLIENTE e só ENVIA (send-only).
//
// Concorrência (ESP32 dual-core):
//   - A NAVEGAÇÃO roda no `loop()` do Arduino (Core 1) e é BLOQUEANTE.
//   - A telemetria (Wi-Fi + WebSocket) roda numa task FIXADA no Core 0,
//     junto da pilha de rede, para NÃO ser travada pelas primitivas de
//     navegação (o que derrubaria a conexão / os giros).
//
// Comunicação entre os núcleos: um "snapshot" do estado do robô protegido por
// mutex. A navegação (Core 1) ESCREVE via telemetriaAtualizar(); a task de
// telemetria (Core 0) LÊ e serializa em JSON no contrato do backend.
//
// IMPORTANTE: telemetriaAtualizar() lê os sensores ToF (I²C). Deve ser chamada
// SEMPRE do Core 1 (contexto da navegação), que é o único dono do barramento
// I²C — a task do Core 0 nunca toca em I²C, só no snapshot já capturado.
// -----------------------------------------------------------------------------

// Conecta o Wi-Fi e sobe a task de telemetria no Core 0. Chamar uma vez no setup.
void telemetriaInit();

// Atualiza o snapshot com o estado atual do robô (chamar do Core 1 / navegação).
//   estado:   string EXATA que o backend reconhece —
//             "EXPLORANDO" (normal), "OBJETIVO_ENCONTRADO" (chegou ao centro),
//             "ERRO" (preso). Estados terminais disparam envio imediato.
//   terminal: true nos estados finais, para forçar o envio na hora.
void telemetriaAtualizar(uint32_t tempoCorridaMs,
                         uint8_t x, uint8_t y, Direcao dir,
                         const char *estado, bool terminal);
