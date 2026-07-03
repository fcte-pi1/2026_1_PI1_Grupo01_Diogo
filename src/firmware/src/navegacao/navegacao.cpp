#include <Arduino.h>
#include "navegacao.h"
#include "pid.h"
#include "../config/pinos.h"
#include "../sensores/imu.h"
#include "../sensores/tof.h"
#include "../sensores/encoders.h"
#include "../atuadores/motores.h"

// =============================================================================
// PARÂMETROS AJUSTÁVEIS  (calibre no seu robô/labirinto)
// =============================================================================
namespace {

// --- Geometria ---
constexpr float   TAMANHO_CELULA_CM = 16.8f;   // lado da célula do labirinto
constexpr float   MARGEM_CELULA_CM  = 0.4f;    // para de andar um pouco antes do alvo

// --- Índices dos sensores ToF (confira com o env teste_tof!) ---
constexpr int     TOF_FRENTE   = 0;
constexpr int     TOF_ESQUERDA = 1;
constexpr int     TOF_DIREITA  = 2;

// --- Limiares de detecção de parede (mm) ---
constexpr uint16_t PAREDE_FRENTE_MM = 80;
constexpr uint16_t PAREDE_LADO_MM   = 140;
constexpr uint16_t PARADA_SEGURA_MM = 50;

// --- Velocidades (PWM 0..255) ---
constexpr int16_t VEL_BASE      = 120;         // cruzeiro na reta (validado na bancada)
constexpr int16_t VEL_CURVA_MAX = 130;         // saturação do giro
constexpr int16_t VEL_CORRECAO_MAX = 80;       // quanto o PID pode desviar da base na reta

// --- Controle ---
constexpr uint16_t INTERVALO_MS = 10;
constexpr float    TOL_ANGULO   = 2.0f;
constexpr float    TOL_VEL_GIRO = 15.0f;
constexpr uint8_t  CICLOS_ESTAVEL = 3;
constexpr unsigned long TIMEOUT_MOV_MS = 10000;

// --- Detecção de stall (rodas travadas: bateu/enroscou sem o ToF frontal pegar) ---
// Backup do PARADA_SEGURA_MM: se os encoders nao avancam STALL_PULSOS_MIN por
// STALL_MS com o robo ja em movimento, encerra em vez de ralar o motor.
constexpr int32_t       STALL_PULSOS_MIN = 3;     // progresso minimo p/ contar como "andando"
constexpr unsigned long STALL_MS         = 150;   // travado por esse tempo => encerra
constexpr int32_t       STALL_ARM_PULSOS = 20;    // so arma depois que o robo saiu do lugar

// --- Ganhos do PID de RETA (heading-hold) ---
// >>> USE AQUI OS MESMOS valores que você validou na bancada teste_reta_imu <<<
// (os abaixo são os recomendados; troque pelos seus se forem diferentes)
constexpr float KP_RETA = 6.0f;
constexpr float KI_RETA = 1.5f;
constexpr float KD_RETA = 0.3f;
constexpr float LIM_INTEGRAL_RETA = 40.0f;

// --- Ganhos do PID de CURVA ---
// >>> A CALIBRAR na bancada teste_giro_imu. Começam BAIXOS de propósito:
//     agora a correção é imediata, então o KP=20 antigo bateria forte demais. <<<
constexpr float KP_CURVA = 5.0f;
constexpr float KI_CURVA = 0.0f;
constexpr float KD_CURVA = 0.3f;
constexpr float LIM_INTEGRAL_CURVA = 200.0f;

// --- Centralização entre paredes (correção suave usando ToF laterais) ---
constexpr float KP_CENTRO = 0.15f;
constexpr int16_t CENTRO_MAX = 30;

// =============================================================================
// Estado interno
// =============================================================================
float anguloAlvoRumo = 0.0f;   // rumo de referência absoluto (graus IMU)

int16_t limitar(int16_t v, int16_t lo, int16_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// Freio ativo: zera cruzeiro/correcao e espera a rampa PARAR de fato os motores.
void frear() {
    motoresParar();
    unsigned long t0 = millis();
    while ((motorLerVelocidadeAtual(MOTOR_ESQUERDO) != 0 ||
            motorLerVelocidadeAtual(MOTOR_DIREITO)  != 0) &&
           millis() - t0 < 1200) {
        motoresAtualizar();
        imuAtualizar();
        delay(2);
    }
}

} // namespace

// =============================================================================
bool navInit() {
    if (!encodersInit()) {
        Serial.println("[NAV] ERRO: encoders.");
        return false;
    }
    anguloAlvoRumo = imuLerAnguloZ();
    Serial.println("[NAV] Navegacao pronta.");
    return true;
}

void navZerarRumo() {
    anguloAlvoRumo = imuLerAnguloZ();
}

// -----------------------------------------------------------------------------
// Leitura de paredes (relativo ao robô)
// -----------------------------------------------------------------------------
bool navParedeFrente() {
    uint16_t d = tofLerDistancia(TOF_FRENTE);
    return (d <= PAREDE_FRENTE_MM);
}
bool navParedeEsquerda() {
    uint16_t d = tofLerDistancia(TOF_ESQUERDA);
    return (d <= PAREDE_LADO_MM);
}
bool navParedeDireita() {
    uint16_t d = tofLerDistancia(TOF_DIREITA);
    return (d <= PAREDE_LADO_MM);
}

// -----------------------------------------------------------------------------
// Avança uma célula mantendo o rumo (PID de heading + centralização opcional)
// Atuador: cruzeiro rampeado (VEL_BASE) + correção imediata (PID + ToF).
// -----------------------------------------------------------------------------
void navAndarUmaCelula() {
    PID pidRumo(KP_RETA, KI_RETA, KD_RETA);
    pidRumo.definirLimiteSaida(-VEL_CORRECAO_MAX, VEL_CORRECAO_MAX);
    pidRumo.definirLimiteIntegral(LIM_INTEGRAL_RETA);
    pidRumo.resetar();

    encodersZerar();
    const float alvoCm = TAMANHO_CELULA_CM - MARGEM_CELULA_CM;

    unsigned long ultimo = millis();
    unsigned long t0     = millis();

    // Estado da detecção de stall.
    int32_t       encAnterior  = 0;      // soma esq+dir na última verificação
    unsigned long ultimoAvanco = t0;     // instante do último progresso dos encoders

    while (true) {
        imuAtualizar();
        motoresAtualizar();

        unsigned long agora = millis();
        if (agora - ultimo < INTERVALO_MS) { delay(1); continue; }
        float dt = (agora - ultimo) / 1000.0f;
        ultimo = agora;

        // Segurança: colidiu / muito perto da parede da frente.
        if (tofLerDistancia(TOF_FRENTE) <= PARADA_SEGURA_MM) {
            Serial.println("[NAV] Parada segura (parede a frente).");
            break;
        }
        // Segurança por tempo.
        if (agora - t0 > TIMEOUT_MOV_MS) {
            Serial.println("[NAV] Timeout ao andar.");
            break;
        }
        // Segurança por stall: rodas paradas com o robô já em movimento => bateu/enroscou.
        int32_t encSoma  = encoderLerEsquerdo() + encoderLerDireito();
        int32_t encDelta = encSoma > encAnterior ? encSoma - encAnterior
                                                  : encAnterior - encSoma;
        if (encDelta >= STALL_PULSOS_MIN) {
            encAnterior  = encSoma;      // houve progresso: rearma o cronômetro
            ultimoAvanco = agora;
        } else if (encSoma > STALL_ARM_PULSOS && agora - ultimoAvanco > STALL_MS) {
            Serial.println("[NAV] Stall: encoders parados, encerrando.");
            break;
        }

        // Distância percorrida (média dos dois encoders).
        float dist = 0.5f * (encoderDistanciaEsquerdaCm() + encoderDistanciaDireitaCm());
        if (dist >= alvoCm) break;

        // --- PID de rumo (IMU) ---
        float ang  = imuLerAnguloZ();
        float erro = anguloAlvoRumo - ang;          // >0 => precisa girar à esquerda
        float correcao = pidRumo.calcular(erro, dt);

        // --- Centralização entre paredes (se houver parede dos dois lados) ---
        float ajusteCentro = 0.0f;
        uint16_t dEsq = tofLerDistancia(TOF_ESQUERDA);
        uint16_t dDir = tofLerDistancia(TOF_DIREITA);
        if (dEsq <= PAREDE_LADO_MM && dDir <= PAREDE_LADO_MM) {
            float e = (float)dDir - (float)dEsq;   // >0 => mais perto da esquerda
            ajusteCentro = KP_CENTRO * e;
            if (ajusteCentro >  CENTRO_MAX) ajusteCentro =  CENTRO_MAX;
            if (ajusteCentro < -CENTRO_MAX) ajusteCentro = -CENTRO_MAX;
        }

        // Cruzeiro rampeado + correção imediata (esq = base - total, dir = base + total).
        int16_t total = (int16_t)(correcao + ajusteCentro);
        motoresSetCruzeiro(VEL_BASE);
        motoresSetCorrecao(total);
    }

    frear();
}

// -----------------------------------------------------------------------------
// Giro por PID até um delta de ângulo (graus). +delta = anti-horário (esquerda).
// Atuador: cruzeiro 0 + correção imediata => rotação pura (esq=-corr, dir=+corr).
// -----------------------------------------------------------------------------
static void navGirarDelta(float delta) {
    anguloAlvoRumo += delta;

    PID pidGiro(KP_CURVA, KI_CURVA, KD_CURVA);
    pidGiro.definirLimiteSaida(-VEL_CURVA_MAX, VEL_CURVA_MAX);
    pidGiro.definirLimiteIntegral(LIM_INTEGRAL_CURVA);
    pidGiro.resetar();

    unsigned long ultimo = millis();
    unsigned long t0     = millis();
    uint8_t estavel = 0;

    while (true) {
        imuAtualizar();
        motoresAtualizar();

        unsigned long agora = millis();
        if (agora - ultimo < INTERVALO_MS) { delay(1); continue; }
        float dt = (agora - ultimo) / 1000.0f;
        ultimo = agora;

        float ang  = imuLerAnguloZ();
        float erro = anguloAlvoRumo - ang;

        // Conclusão: dentro da tolerância e girando devagar por alguns ciclos.
        if (fabsf(erro) <= TOL_ANGULO && fabsf(imuLerGiroZ()) <= TOL_VEL_GIRO) {
            if (++estavel >= CICLOS_ESTAVEL) break;
        } else {
            estavel = 0;
        }

        if (agora - t0 > TIMEOUT_MOV_MS) {
            Serial.println("[NAV] Timeout ao girar.");
            break;
        }

        float saida = pidGiro.calcular(erro, dt);

        // erro>0 (girar à ESQUERDA/anti-horário): esq p/ trás, dir p/ frente.
        motoresSetCruzeiro(0);
        motoresSetCorrecao((int16_t)saida);
    }

    frear();
}

void navGirarDireita()   { navGirarDelta(-90.0f); }  // horário  = ângulo diminui
void navGirarEsquerda()  { navGirarDelta(+90.0f); }  // anti-horário = ângulo aumenta
void navGirarMeiaVolta() { navGirarDelta(+180.0f); }

void navParar() {
    motoresParar();
    motoresAtualizar();
}
