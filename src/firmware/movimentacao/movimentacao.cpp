#include <Arduino.h>
#include "movimentacao.h"
#include "../sensores/imu.h"
#include "../atuadores/motores.h"

// ---------------------------------------------------------------------------
// Constantes de tuning — ajustar na bancada após calibrar o IMU
// ---------------------------------------------------------------------------
namespace {

// --- Giro (pivot turn) ---
// PID P+D sobre o erro de yaw em graus.
// saída → PWM diferencial entre motores (esquerdo = -saida, direito = +saida).
constexpr float KP_GIRO    = 2.5f;   // ganho proporcional
constexpr float KD_GIRO    = 0.08f;  // ganho derivativo
constexpr float TOL_GRAUS  = 2.0f;   // tolerância para considerar giro concluído (°)
constexpr float PWM_GIRO_MIN = 40.0f; // PWM mínimo para vencer atrito estático
constexpr float PWM_GIRO_MAX = 100.0f;// PWM máximo durante o giro
constexpr uint32_t TIMEOUT_GIRO_MS = 3000; // timeout de segurança (ms)

// --- Avanço reto ---
// Open-loop temporal com correção de yaw proporcional.
// TODO(#73): substituir por controle por odometria quando disponível.
constexpr float    PWM_AVANCO    = 150.0f; // PWM base de avanço
constexpr float    KP_RETO      = 1.2f;   // ganho de correção yaw (PWM/grau)
constexpr uint32_t CELULA_MS    = 500;    // duração de uma célula (~18 cm) a PWM 150

// --- Estabilização ---
constexpr uint32_t RAMPA_ESPERA_MS = 250; // aguarda rampa de aceleração antes de integrar yaw

} // namespace

// ---------------------------------------------------------------------------
// Helpers internos
// ---------------------------------------------------------------------------

/**
 * Integra o yaw a partir do giroscópio Z durante `duracaoMs` milissegundos.
 * Retorna o ângulo acumulado em graus (positivo = esquerda).
 *
 * Usa o período de amostragem real para minimizar o drift de integração.
 */
static float integrarYaw(uint32_t duracaoMs) {
    float yaw = 0.0f;
    uint32_t tAnterior = micros();
    const uint32_t tFim = millis() + duracaoMs;

    while (millis() < tFim) {
        motoresAtualizar();
        uint32_t agora = micros();
        float dt = (agora - tAnterior) * 1e-6f; // µs → s
        tAnterior = agora;
        yaw += imuLerGiroZ() * dt;
        delayMicroseconds(500); // ~2 kHz de amostragem
    }
    return yaw;
}

/**
 * Limita um float ao intervalo [minVal, maxVal].
 */
static float limitar(float valor, float minVal, float maxVal) {
    if (valor > maxVal) return maxVal;
    if (valor < minVal) return minVal;
    return valor;
}

/**
 * Garante PWM mínimo para vencer atrito, preservando o sinal.
 */
static float aplicarPwmMinimo(float saida) {
    if (saida > 0.0f && saida < PWM_GIRO_MIN) return PWM_GIRO_MIN;
    if (saida < 0.0f && saida > -PWM_GIRO_MIN) return -PWM_GIRO_MIN;
    return saida;
}

/**
 * Executa um pivot turn (giro no próprio eixo) até o ângulo alvo.
 *
 * Convenção: alvoGraus > 0 → esquerda; alvoGraus < 0 → direita.
 * Motores: esquerdo = -saida, direito = +saida.
 *   - saida > 0 (erro > 0, ainda falta girar à esquerda):
 *       ESQUERDO recua, DIREITO avança → robô gira à esquerda ✓
 *   - saida < 0 (erro < 0, ainda falta girar à direita):
 *       ESQUERDO avança, DIREITO recua → robô gira à direita ✓
 */
static void girarPorAngulo(float alvoGraus) {
    float yaw       = 0.0f;
    float erroAnt   = alvoGraus;
    uint32_t tAnt   = micros();
    const uint32_t tFim = millis() + TIMEOUT_GIRO_MS;
    uint32_t tDentroTol = 0;

    while (millis() < tFim) {
        motoresAtualizar();

        uint32_t agora = micros();
        float dt = (agora - tAnt) * 1e-6f;
        tAnt = agora;

        yaw += imuLerGiroZ() * dt;

        const float erro    = alvoGraus - yaw;
        const float derivada = (erro - erroAnt) / dt;
        erroAnt = erro;

        float saida = KP_GIRO * erro + KD_GIRO * derivada;
        saida = aplicarPwmMinimo(saida);
        saida = limitar(saida, -PWM_GIRO_MAX, PWM_GIRO_MAX);

        motorSetVelocidade(MOTOR_ESQUERDO, -(int16_t)saida);
        motorSetVelocidade(MOTOR_DIREITO,   (int16_t)saida);

        // Verifica tolerância por 100 ms contínuos antes de declarar conclusão
        if (fabsf(erro) <= TOL_GRAUS) {
            if (tDentroTol == 0) tDentroTol = millis();
            if (millis() - tDentroTol >= 100) break;
        } else {
            tDentroTol = 0;
        }

        delayMicroseconds(500);
    }

    motoresParar();

    if (millis() >= tFim) {
        Serial.printf("[Mov] AVISO: timeout no giro (alvo=%.1f°, yaw=%.1f°)\n",
                      alvoGraus, yaw);
    } else {
        Serial.printf("[Mov] Giro concluido: alvo=%.1f° yaw=%.1f°\n", alvoGraus, yaw);
    }

    // Aguarda a rampa de desaceleração zerar os motores
    delay(200);
}

// ---------------------------------------------------------------------------
// API pública
// ---------------------------------------------------------------------------

bool movimentacaoInit() {
    // Valida que os subsistemas de que dependemos estão prontos.
    // motoresInit() e imuInit() devem ter sido chamados antes.
    Serial.println("[Mov] Modulo de movimentacao pronto");
    return true;
}

void avancarCelula() {
    Serial.println("[Mov] avancarCelula — open-loop temporal");

    // Zera referência de yaw
    float yaw     = 0.0f;
    uint32_t tAnt = micros();

    // Define velocidade base e aguarda rampa estabilizar
    motorSetVelocidade(MOTOR_ESQUERDO, (int16_t)PWM_AVANCO);
    motorSetVelocidade(MOTOR_DIREITO,  (int16_t)PWM_AVANCO);

    const uint32_t tInicio = millis();
    const uint32_t tFim    = tInicio + CELULA_MS;

    while (millis() < tFim) {
        motoresAtualizar();

        uint32_t agora = micros();
        float dt = (agora - tAnt) * 1e-6f;
        tAnt = agora;
        yaw += imuLerGiroZ() * dt;

        // Correção proporcional de yaw
        // yaw > 0 → robô desviou à esquerda → acelera esquerdo, freia direito
        // yaw < 0 → robô desviou à direita  → freia esquerdo, acelera direito
        if (millis() - tInicio > RAMPA_ESPERA_MS) {
            const float correcao = KP_RETO * yaw;
            const float pwmEsq = limitar(PWM_AVANCO + correcao, 0.0f, 255.0f);
            const float pwmDir = limitar(PWM_AVANCO - correcao, 0.0f, 255.0f);
            motorSetVelocidade(MOTOR_ESQUERDO, (int16_t)pwmEsq);
            motorSetVelocidade(MOTOR_DIREITO,  (int16_t)pwmDir);
        }

        delayMicroseconds(500);
    }

    motoresParar();
    Serial.printf("[Mov] Celula concluida (yaw final=%.1f°)\n", yaw);
    delay(100);
}

void girarDireita90() {
    Serial.println("[Mov] girarDireita90");
    // Direita = ângulo negativo (horário visto de cima)
    girarPorAngulo(-90.0f);
}

void girarEsquerda90() {
    Serial.println("[Mov] girarEsquerda90");
    // Esquerda = ângulo positivo (anti-horário visto de cima)
    girarPorAngulo(+90.0f);
}

void pararMovimentacao() {
    motoresParar();
    Serial.println("[Mov] pararMovimentacao");
}
