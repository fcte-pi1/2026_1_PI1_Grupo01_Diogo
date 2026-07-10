#include "comunicacao/telemetria_contrato.h"

#include <cstring>
#include <ArduinoJson.h>

namespace telemetria {

const char *dirTexto(uint8_t d) {
    switch (d) {
        case 0: return "NORTE";
        case 1: return "LESTE";
        case 2: return "SUL";
        case 3: return "OESTE";
    }
    return "NORTE";
}

bool estadoTerminal(const char *estado) {
    if (!estado) return false;
    return std::strcmp(estado, "OBJETIVO_ENCONTRADO") == 0
        || std::strcmp(estado, "CONCLUIDO") == 0
        || std::strcmp(estado, "ERRO") == 0;
}

std::string montarEnvelope(const Snapshot &s) {
    JsonDocument doc;
    doc["type"] = "telemetria";

    JsonObject p = doc["payload"].to<JsonObject>();
    p["tempo_corrida_ms"] = s.tempoCorridaMs;
    p["posicao_x"]        = s.x;
    p["posicao_y"]        = s.y;
    p["direcao_atual"]    = dirTexto(s.dir);
    p["estado_robo"]      = s.estado;
    p["bateria_pct"]      = s.bateriaPct;

    JsonObject sensores = p["leitura_sensores"].to<JsonObject>();
    sensores["dist_frente_cm"]   = s.frenteMm / 10.0;
    sensores["dist_esquerda_cm"] = s.esqMm / 10.0;
    sensores["dist_direita_cm"]  = s.dirMm / 10.0;

    std::string out;
    serializeJson(doc, out);
    return out;
}

}  // namespace telemetria
