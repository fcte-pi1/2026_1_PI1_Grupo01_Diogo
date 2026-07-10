// -----------------------------------------------------------------------------
// Teste NATIVE (roda no PC, sem ESP) da lógica pura do contrato de telemetria.
//
//   pio test -e native
//
// Valida que o firmware gera EXATAMENTE o contrato que o backend consome:
// campos obrigatórios, conversão mm->cm, mapeamento de direção e detecção de
// estado terminal. Fecha o laço com os testes do backend (que validam o parse).
// -----------------------------------------------------------------------------

#include <unity.h>
#include <string>
#include <ArduinoJson.h>

#include "comunicacao/telemetria_contrato.h"

using telemetria::Snapshot;
using telemetria::dirTexto;
using telemetria::estadoTerminal;
using telemetria::montarEnvelope;

void setUp() {}
void tearDown() {}

// Snapshot de referência usado em vários casos.
static Snapshot exemplo() {
    Snapshot s;
    s.tempoCorridaMs = 5000;
    s.x = 3;
    s.y = 2;
    s.dir = 1;                 // LESTE
    s.estado = "EXPLORANDO";
    s.frenteMm = 120;          // -> 12.0 cm
    s.esqMm = 40;              // ->  4.0 cm
    s.dirMm = 70;              // ->  7.0 cm
    s.bateriaPct = 100;
    return s;
}

// --- dirTexto -----------------------------------------------------------------
void test_dirTexto_mapeia_as_quatro_direcoes() {
    TEST_ASSERT_EQUAL_STRING("NORTE", dirTexto(0));
    TEST_ASSERT_EQUAL_STRING("LESTE", dirTexto(1));
    TEST_ASSERT_EQUAL_STRING("SUL",   dirTexto(2));
    TEST_ASSERT_EQUAL_STRING("OESTE", dirTexto(3));
}

void test_dirTexto_fallback_para_valor_invalido() {
    TEST_ASSERT_EQUAL_STRING("NORTE", dirTexto(99));
}

// --- estadoTerminal -----------------------------------------------------------
void test_estadoTerminal_reconhece_terminais() {
    TEST_ASSERT_TRUE(estadoTerminal("OBJETIVO_ENCONTRADO"));
    TEST_ASSERT_TRUE(estadoTerminal("CONCLUIDO"));
    TEST_ASSERT_TRUE(estadoTerminal("ERRO"));
}

void test_estadoTerminal_ignora_nao_terminais() {
    TEST_ASSERT_FALSE(estadoTerminal("EXPLORANDO"));
    TEST_ASSERT_FALSE(estadoTerminal("INICIANDO"));
    TEST_ASSERT_FALSE(estadoTerminal(nullptr));
}

// --- montarEnvelope: estrutura e campos --------------------------------------
void test_envelope_tem_type_e_payload() {
    std::string json = montarEnvelope(exemplo());

    JsonDocument doc;
    TEST_ASSERT_FALSE(deserializeJson(doc, json));

    TEST_ASSERT_EQUAL_STRING("telemetria", (const char *)doc["type"]);
    TEST_ASSERT_TRUE(doc["payload"].is<JsonObject>());
}

void test_envelope_mapeia_campos_obrigatorios() {
    std::string json = montarEnvelope(exemplo());
    JsonDocument doc;
    deserializeJson(doc, json);
    JsonObject p = doc["payload"];

    // Campos que o validador do backend exige (INVALID_PAYLOAD se faltarem).
    TEST_ASSERT_EQUAL_UINT32(5000, (uint32_t)p["tempo_corrida_ms"]);
    TEST_ASSERT_EQUAL_INT(3, (int)p["posicao_x"]);
    TEST_ASSERT_EQUAL_INT(2, (int)p["posicao_y"]);
    TEST_ASSERT_EQUAL_INT(100, (int)p["bateria_pct"]);

    TEST_ASSERT_EQUAL_STRING("LESTE", (const char *)p["direcao_atual"]);
    TEST_ASSERT_EQUAL_STRING("EXPLORANDO", (const char *)p["estado_robo"]);
}

void test_envelope_converte_mm_para_cm() {
    std::string json = montarEnvelope(exemplo());
    JsonDocument doc;
    deserializeJson(doc, json);
    JsonVariant sensores = doc["payload"]["leitura_sensores"];

    TEST_ASSERT_TRUE(sensores.is<JsonObject>());
    TEST_ASSERT_EQUAL_FLOAT(12.0, sensores["dist_frente_cm"].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(4.0,  sensores["dist_esquerda_cm"].as<float>());
    TEST_ASSERT_EQUAL_FLOAT(7.0,  sensores["dist_direita_cm"].as<float>());
}

void test_envelope_propaga_estado_terminal() {
    Snapshot s = exemplo();
    s.estado = "OBJETIVO_ENCONTRADO";
    std::string json = montarEnvelope(s);
    JsonDocument doc;
    deserializeJson(doc, json);

    TEST_ASSERT_EQUAL_STRING("OBJETIVO_ENCONTRADO",
                             (const char *)doc["payload"]["estado_robo"]);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_dirTexto_mapeia_as_quatro_direcoes);
    RUN_TEST(test_dirTexto_fallback_para_valor_invalido);
    RUN_TEST(test_estadoTerminal_reconhece_terminais);
    RUN_TEST(test_estadoTerminal_ignora_nao_terminais);
    RUN_TEST(test_envelope_tem_type_e_payload);
    RUN_TEST(test_envelope_mapeia_campos_obrigatorios);
    RUN_TEST(test_envelope_converte_mm_para_cm);
    RUN_TEST(test_envelope_propaga_estado_terminal);
    return UNITY_END();
}
