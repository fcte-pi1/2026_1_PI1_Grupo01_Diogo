#pragma once

#include <cstdint>
#include <string>

// -----------------------------------------------------------------------------
// Lógica PURA do contrato de telemetria — sem Wi-Fi, WebSocket, FreeRTOS ou
// Arduino. Depende só de ArduinoJson (header-only, compila no host).
//
// É esta parte que produz o JSON no formato que o backend espera. Por ser
// isolada, roda tanto na ESP (build de produção) quanto no PC (teste `native`),
// garantindo que o firmware gera EXATAMENTE o contrato que o backend valida —
// sem precisar gravar nada numa ESP.
// -----------------------------------------------------------------------------

namespace telemetria {

// Fotografia do estado do robô a ser transmitida.
struct Snapshot {
    uint32_t    tempoCorridaMs;
    uint8_t     x;
    uint8_t     y;
    uint8_t     dir;        // 0=NORTE, 1=LESTE, 2=SUL, 3=OESTE
    const char *estado;     // "EXPLORANDO" | "OBJETIVO_ENCONTRADO" | "ERRO" ...
    uint16_t    frenteMm;
    uint16_t    esqMm;
    uint16_t    dirMm;
    uint8_t     bateriaPct;
};

// Nome absoluto da direção a partir do código (0..3). Fallback: "NORTE".
const char *dirTexto(uint8_t d);

// true se o estado encerra a corrida (o backend finaliza ao recebê-lo) e o
// firmware deve enviar imediatamente. Chaves: OBJETIVO_ENCONTRADO, CONCLUIDO, ERRO.
bool estadoTerminal(const char *estado);

// Monta o envelope { "type":"telemetria", "payload":{...} } serializado em JSON,
// com a conversão mm -> cm dos sensores.
std::string montarEnvelope(const Snapshot &s);

}  // namespace telemetria
