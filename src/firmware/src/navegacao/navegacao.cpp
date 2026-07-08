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
constexpr float   TAMANHO_CELULA_CM = 18.0f;   // pitch: quanto o robô anda por célula (16,8 é a largura interna)
constexpr float   MARGEM_CELULA_CM  = 2.4f;    // para o motor antes do alvo p/ o deslize do freio completar os 18 cm (piso liso)

// --- Índices dos sensores ToF (confira com o env teste_tof!) ---
constexpr int     TOF_FRENTE   = 0;
constexpr int     TOF_ESQUERDA = 1;
constexpr int     TOF_DIREITA  = 2;

// --- Limiares de detecção de parede (mm) ---
constexpr uint16_t PAREDE_FRENTE_MM = 80;
constexpr uint16_t PAREDE_LADO_MM   = 140;
constexpr uint16_t PARADA_SEGURA_MM = 50;

// --- Velocidades (PWM 0..255) ---
constexpr int16_t VEL_BASE      = 85;          // cruzeiro na reta (baixado 100->85: + tempo/cm p/ corrigir => menos serpenteio, menos risco de raspar. 120 deslizava no freio)
constexpr int16_t VEL_CURVA_MAX = 150;         // saturação do giro (validado teste_giro_stress)
constexpr int16_t VEL_CORRECAO_MAX = 80;       // quanto o PID pode desviar da base na reta

// --- Reta: desaceleração + freio ativo (portado do teste_centralizacao validado) ---
constexpr int16_t       VEL_MIN_RETA  = 65;    // piso da rampa de decel (atrito do labirinto)
constexpr float         DECEL_RETA_CM = 8.0f;  // desacelera nos últimos cm
constexpr unsigned long FREIO_RETA_MS = 250;   // duração do freio ativo (short-brake)
constexpr int16_t       VEL_ARRANCADA = 70;    // empurrão inicial do cruzeiro (evita giro no lugar na saída)
constexpr float         RAMPA_RUMO_CM = 8.0f;  // espalha a conclusão do viés na saída nesses cm (curva suave)

// --- Controle ---
constexpr uint16_t INTERVALO_MS = 10;
constexpr float    TOL_ANGULO   = 4.5f;   // tolerância de conclusão do giro (validado)
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

// --- Giro com VIÉS (portado do teste_giro_stress / teste_curva_planoA validados) ---
// A bússola recebe o ±90 CHEIO; o giro FÍSICO para em (±90 − VIES) -> resíduo cai
// PRA FORA (lado seguro) e a reta seguinte completa os últimos graus.
constexpr float KP_CURVA = 5.0f;
constexpr float KI_CURVA = 8.0f;
constexpr float KD_CURVA = 0.3f;
constexpr float LIM_INTEGRAL_CURVA = 15.0f;
float VIES_CURVA = 11.0f;   // graus que o giro para ANTES do alvo (pra fora). VALIDADO 07/08
                            // nos 2 lados. Tunável (navDefinirViesCurva) p/ bateria/venue: cheia desliza mais.
constexpr float PWM_MIN_GIRO = 90.0f;   // piso: vence o atrito estático
constexpr float GRAUS_DECEL  = 40.0f;   // começa a desacelerar a este erro angular
constexpr float VEL_GIRO_MIN = 95.0f;   // piso do teto na desaceleração angular

// --- Esquadro por toque (Fase 1 do Plano A): anda até ENCOSTAR na parede ---
// Toque SUAVE (validado no teste_curva_planoA refatorado): não bate forte.
constexpr int16_t       PWM_APROX           = 90;   // avanço em direção à parede (longe)
constexpr int16_t       PWM_TOQUE           = 38;   // creep suave perto da parede (baixado 45->38: bateria cheia batia)
constexpr int16_t       PWM_CONFIRM         = 70;   // empurrão de confirmação (baixado 80->70: bate menos)
constexpr uint16_t      TOF_CREEP_MM        = 150;  // ToF frontal < isto -> entra em creep (latch)
constexpr uint16_t      TOF_DECEL_MM        = 350;  // acima do creep: desacelera a aproximacao (PWM_APROX->PWM_TOQUE) p/ nao bater forte vindo de longe
constexpr unsigned long STALL_CONFIRM_MS    = 150;  // parado empurrando -> TOCOU
constexpr unsigned long ESQUADRO_TIMEOUT_MS = 5000; // não tocou nesse tempo -> desiste

// --- Centralização em CASCATA (posição lateral -> viés no setpoint de rumo) ---
// Validado no teste_centralizacao (07/07). Gate por parede dos 2 lados (dist.
// CORRIGIDA <= PAREDE_LADO_MM) + sanidade ToF. Sinal: viés < 0 aponta nariz p/ direita.
constexpr float    DIF_ALVO       = -55.0f;    // (esq_corr - dir_cru) no CENTRO (medido ~-60)
// Amaciados 07/08 (serpenteava): a cascata é P puro -> propensa a ciclo-limite.
// TUNÁVEIS ao vivo (navDefinirCentro*) p/ dialar contra bateria/venue.
float    KP_POS         = 0.07f;    // graus de viés por unidade de difC (menor=mais suave/lento)
float    VIES_RUMO_MAX  = 5.0f;     // teto do viés de rumo (graus) (menor=excursões menores)
float    DEADBAND_POS   = 14.0f;    // zona morta em unid. de difC ~9mm (maior=para de cutucar antes)
constexpr uint16_t TOF_MIN_VALIDO = 1;         // sanidade ToF: rejeita 0
constexpr uint16_t TOF_MAX_VALIDO = 2000;      // sanidade ToF: rejeita 8191/lixo/aberto longe

// =============================================================================
// Estado interno
// =============================================================================
float anguloAlvoRumo = 0.0f;   // rumo de referência absoluto (graus IMU)

int16_t limitar(int16_t v, int16_t lo, int16_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// Sanidade do ToF: rejeita 0 e códigos de erro/lixo (8191, >2000 mm).
inline bool tofValido(uint16_t v) { return v >= TOF_MIN_VALIDO && v <= TOF_MAX_VALIDO; }

// Leitura de parede robusta (Inc 5a): tira N amostras frescas (ToF em modo
// contínuo bloqueia até ter dado novo), descarta as inválidas (0/8191/>2000 —
// o glitch esporádico da direita, HANDOFF §8) e devolve a MEDIANA das válidas.
// Imune ao valor falso isolado. Se NENHUMA for válida (falha persistente),
// devolve UINT16_MAX -> tratado como "sem parede" (não inventa parede fantasma,
// que travaria o flood fill; um falso-aberto ainda tem os backstops físicos da
// primitiva: parada-segura frontal + stall).
uint16_t tofDistanciaParede(int idx) {
    constexpr int N = 5;
    uint16_t v[N];
    int n = 0;
    for (int i = 0; i < N; i++) {
        uint16_t d = tofLerDistancia(idx);
        if (tofValido(d)) v[n++] = d;
    }
    if (n == 0) return UINT16_MAX;
    for (int i = 1; i < n; i++) {          // insertion sort (n <= 5)
        uint16_t k = v[i]; int j = i - 1;
        while (j >= 0 && v[j] > k) { v[j + 1] = v[j]; j--; }
        v[j + 1] = k;
    }
    return v[n / 2];
}

// Freio ATIVO (short-brake): curto-circuita os motores pra matar a inércia de
// vez, espera assentar e libera. Usar já EM BAIXA velocidade (após a decel).
void frearAtivo() {
    motoresFrear();
    unsigned long t0 = millis();
    while (millis() - t0 < FREIO_RETA_MS) {
        imuAtualizar();
        delay(2);
    }
    motoresParar();
    motoresAtualizar();
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
bool navParedeFrente()   { return tofDistanciaParede(TOF_FRENTE)   <= PAREDE_FRENTE_MM; }
bool navParedeEsquerda() { return tofDistanciaParede(TOF_ESQUERDA) <= PAREDE_LADO_MM;   }
bool navParedeDireita()  { return tofDistanciaParede(TOF_DIREITA)  <= PAREDE_LADO_MM;   }

// -----------------------------------------------------------------------------
// FASE 1 do Plano A: anda pra frente (heading-hold + centralização) até TOCAR a
// parede da frente (stall dos encoders confirmado). Esquadra o rumo e minimiza o
// offset do eixo antes de girar -> a roda traseira não raspa. Toque SUAVE.
// (Portado do teste_curva_planoA.) Retorna true se ENCOSTOU; false se timeout.
// -----------------------------------------------------------------------------
bool navEsquadrar() {
    PID pidRumo(KP_RETA, KI_RETA, KD_RETA);
    pidRumo.definirLimiteSaida(-VEL_CORRECAO_MAX, VEL_CORRECAO_MAX);
    pidRumo.definirLimiteIntegral(LIM_INTEGRAL_RETA);
    pidRumo.resetar();

    encodersZerar();

    const unsigned long t0 = millis();
    unsigned long ultimo        = t0;
    int32_t       encAnterior   = 0;
    unsigned long ultimoAvanco  = t0;
    bool          emCreep       = false;   // latch: uma vez perto, fica devagar até encostar
    bool          suspeito      = false;   // stall candidato -> empurra p/ confirmar
    unsigned long suspeitoDesde = 0;
    bool          tocou         = false;

    while (true) {
        imuAtualizar();
        motoresAtualizar();

        const unsigned long agora = millis();
        if (agora - ultimo < INTERVALO_MS) { delay(1); continue; }
        const float dt = (agora - ultimo) / 1000.0f;
        ultimo = agora;

        const int32_t eEsq = encoderLerEsquerdo();
        const int32_t eDir = encoderLerDireito();

        // Stall = travou. Pode ser a PAREDE ou o atrito vencendo o creep fraco; ao
        // suspeitar, empurra a PWM_CONFIRM e só confirma o toque se continuar parado.
        const int32_t encSoma  = eEsq + eDir;
        const int32_t encDelta = encSoma > encAnterior ? encSoma - encAnterior
                                                       : encAnterior - encSoma;
        if (encDelta >= STALL_PULSOS_MIN) {
            encAnterior = encSoma; ultimoAvanco = agora; suspeito = false;
        } else if (encSoma > STALL_ARM_PULSOS && agora - ultimoAvanco > STALL_MS) {
            if (!suspeito) { suspeito = true; suspeitoDesde = agora; }
            if (agora - suspeitoDesde > STALL_CONFIRM_MS) { tocou = true; break; }
        }

        if (agora - t0 > ESQUADRO_TIMEOUT_MS) {
            Serial.println("[NAV] Esquadro: timeout (nao tocou).");
            break;
        }

        // Heading-hold na bússola + centralização em cascata na aproximação.
        const float ang = imuLerAnguloZ();
        float viesRumo = 0.0f;
        uint16_t esqCorr = tofLerDistancia(TOF_ESQUERDA);
        uint16_t dirCru  = tofLerDistanciaBruta(TOF_DIREITA);
        uint16_t dirCorr = tofLerDistancia(TOF_DIREITA);
        if (tofValido(esqCorr) && esqCorr <= PAREDE_LADO_MM &&
            tofValido(dirCorr) && dirCorr <= PAREDE_LADO_MM && tofValido(dirCru)) {
            float erroPos = ((float)esqCorr - (float)dirCru) - DIF_ALVO;
            float erroEff = fabsf(erroPos) <= DEADBAND_POS ? 0.0f
                            : erroPos - (erroPos > 0.0f ? DEADBAND_POS : -DEADBAND_POS);
            viesRumo = KP_POS * erroEff;
            if (viesRumo >  VIES_RUMO_MAX) viesRumo =  VIES_RUMO_MAX;
            if (viesRumo < -VIES_RUMO_MAX) viesRumo = -VIES_RUMO_MAX;
        }
        const float correcao = pidRumo.calcular((anguloAlvoRumo + viesRumo) - ang, dt);

        // Velocidade: arranca em PWM_APROX; entra em creep (latch) só DEPOIS de ter
        // andado (garante breakaway). No stall suspeito, empurra em PWM_CONFIRM.
        const uint16_t df = tofLerDistancia(TOF_FRENTE);
        if (df > 0 && df < TOF_CREEP_MM && encSoma > STALL_ARM_PULSOS) emCreep = true;
        // Velocidade de aproximacao: no creep = toque suave; entre TOF_DECEL_MM e
        // TOF_CREEP_MM desacelera de PWM_APROX ate PWM_TOQUE (chega devagar mesmo
        // vindo de longe -> nao bate forte); acima disso = PWM_APROX cheio.
        int16_t vel;
        if (emCreep) {
            vel = PWM_TOQUE;
        } else if (df > 0 && df < TOF_DECEL_MM) {
            float frac = (float)(df - TOF_CREEP_MM) / (float)(TOF_DECEL_MM - TOF_CREEP_MM);
            if (frac < 0.0f) frac = 0.0f;
            vel = PWM_TOQUE + (int16_t)((PWM_APROX - PWM_TOQUE) * frac);
        } else {
            vel = PWM_APROX;
        }
        if (suspeito) vel = PWM_CONFIRM;

        motoresSetCruzeiro(vel);
        motoresSetCorrecao((int16_t)correcao);
    }

    frearAtivo();

    // Re-referência física do rumo (Inc 5c): se ENCOSTOU, o toque assenta o robô
    // esquadrado na parede -> rumo físico conhecido. Reancorar a bússola aqui zera
    // a deriva acumulada da IMU a cada junção (bounda por corredor, não deixa
    // acumular no labirinto). Só se tocou — timeout não dá referência confiável.
    if (tocou) navZerarRumo();
    return tocou;
}

// -----------------------------------------------------------------------------
// Avança uma célula mantendo o rumo (heading-hold) com CENTRALIZAÇÃO EM CASCATA
// (posição lateral -> viés no setpoint de rumo), desaceleração nos últimos cm e
// freio ativo no fim. Distância medida pelos encoders. (Portado do teste_centralizacao.)
// re=true: anda de RÉ (beco/180 — não gira no lugar). Sem parada-segura frontal e
// com o viés da centralização INVERTIDO (de ré, a mesma inclinação translada oposto).
// -----------------------------------------------------------------------------
void navAndarUmaCelula(bool re) {
    PID pidRumo(KP_RETA, KI_RETA, KD_RETA);
    pidRumo.definirLimiteSaida(-VEL_CORRECAO_MAX, VEL_CORRECAO_MAX);
    pidRumo.definirLimiteIntegral(LIM_INTEGRAL_RETA);
    pidRumo.resetar();

    encodersZerar();
    const float alvoCm = TAMANHO_CELULA_CM - MARGEM_CELULA_CM;

    // Empurrão inicial: arranca o cruzeiro direto em VEL_ARRANCADA (sem rampar do 0)
    // pra a correção da largada virar uma CURVA suave, não um giro no lugar (arrancada).
    motoresIniciarCruzeiro(re ? -VEL_ARRANCADA : VEL_ARRANCADA);

    // Rumo de partida: se veio de um giro (saída), fica ~viés curto do alvo cheio; a
    // rampa abaixo espalha a conclusão desse resíduo ao longo de RAMPA_RUMO_CM.
    const float angInicial = imuLerAnguloZ();

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

        // Segurança: muito perto da parede da frente. Só no AVANÇO — de ré a parede
        // da frente é justamente a que ele está deixando pra trás, então ignora.
        if (!re && tofLerDistancia(TOF_FRENTE) <= PARADA_SEGURA_MM) {
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
        int32_t encMag   = encSoma < 0 ? -encSoma : encSoma;   // magnitude (funciona no ré)
        if (encDelta >= STALL_PULSOS_MIN) {
            encAnterior  = encSoma;      // houve progresso: rearma o cronômetro
            ultimoAvanco = agora;
        } else if (encMag > STALL_ARM_PULSOS && agora - ultimoAvanco > STALL_MS) {
            Serial.println("[NAV] Stall: encoders parados, encerrando.");
            break;
        }

        // Distância percorrida (média dos dois encoders; |...| p/ funcionar no ré).
        float dist = fabsf(0.5f * (encoderDistanciaEsquerdaCm() + encoderDistanciaDireitaCm()));
        if (dist >= alvoCm) break;

        // --- Centralização em CASCATA: erro de posição -> viés no setpoint de rumo ---
        // (só com parede dos 2 lados, pela dist. CORRIGIDA, + sanidade ToF).
        float viesRumo = 0.0f;
        uint16_t esqCorr = tofLerDistancia(TOF_ESQUERDA);       // esq corrigida (sinal)
        uint16_t dirCru  = tofLerDistanciaBruta(TOF_DIREITA);   // dir CRUA (sinal)
        uint16_t dirCorr = tofLerDistancia(TOF_DIREITA);        // dir corrigida (só p/ gatear)
        if (tofValido(esqCorr) && esqCorr <= PAREDE_LADO_MM &&
            tofValido(dirCorr) && dirCorr <= PAREDE_LADO_MM && tofValido(dirCru)) {
            float erroPos = ((float)esqCorr - (float)dirCru) - DIF_ALVO;
            float erroEff = fabsf(erroPos) <= DEADBAND_POS ? 0.0f
                            : erroPos - (erroPos > 0.0f ? DEADBAND_POS : -DEADBAND_POS);
            viesRumo = KP_POS * erroEff;
            if (viesRumo >  VIES_RUMO_MAX) viesRumo =  VIES_RUMO_MAX;
            if (viesRumo < -VIES_RUMO_MAX) viesRumo = -VIES_RUMO_MAX;
        }
        // De RÉ, a mesma inclinação de nariz translada o robô pro lado OPOSTO (ele vai
        // na direção da traseira) -> inverte o viés pra centralização puxar pro certo.
        if (re) viesRumo = -viesRumo;

        // --- Alvo de rumo RAMPADO: espalha a conclusão do viés (de angInicial até o
        // rumo cheio) ao longo de RAMPA_RUMO_CM -> curva suave e simétrica na saída,
        // em vez de brusca na largada. Célula já alinhada: angInicial≈alvo -> inócuo.
        float frac = dist / RAMPA_RUMO_CM;
        if (frac > 1.0f) frac = 1.0f;
        const float alvoRampa = angInicial + (anguloAlvoRumo - angInicial) * frac;

        // --- PID de rumo: alvo rampado + viés da centralização ---
        float ang  = imuLerAnguloZ();
        float erro = (alvoRampa + viesRumo) - ang;   // >0 => precisa girar à esquerda
        float correcao = pidRumo.calcular(erro, dt);

        // Perfil de velocidade: desacelera nos últimos DECEL_RETA_CM.
        int16_t vel = VEL_BASE;
        float restante = alvoCm - dist;
        if (restante < DECEL_RETA_CM) {
            vel = VEL_MIN_RETA + (int16_t)((VEL_BASE - VEL_MIN_RETA) * (restante / DECEL_RETA_CM));
            if (vel < VEL_MIN_RETA) vel = VEL_MIN_RETA;
        }
        motoresSetCruzeiro(re ? -vel : vel);
        motoresSetCorrecao((int16_t)correcao);
    }

    frearAtivo();
}

// -----------------------------------------------------------------------------
// Giro por PID até um delta de ângulo (graus). +delta = anti-horário (esquerda).
// Atuador: cruzeiro 0 + correção imediata => rotação pura (esq=-corr, dir=+corr).
// -----------------------------------------------------------------------------
static void navGirarDelta(float delta) {
    // A bússola recebe o rumo CHEIO (não corrompe o mapa). O giro FÍSICO mira um
    // tico ANTES (viés) -> o resíduo cai PRA FORA; a reta seguinte (que mira na
    // bússola cheia) completa os últimos graus vindo do lado seguro.
    anguloAlvoRumo += delta;
    const float sinal    = (delta >= 0.0f) ? 1.0f : -1.0f;
    const float alvoGiro = anguloAlvoRumo - sinal * VIES_CURVA;

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

        float ang     = imuLerAnguloZ();
        float velGiro = imuLerGiroZ();
        float erro    = alvoGiro - ang;

        // Conclusão: dentro da tolerância e girando devagar por alguns ciclos.
        if (fabsf(erro) <= TOL_ANGULO && fabsf(velGiro) <= TOL_VEL_GIRO) {
            if (++estavel >= CICLOS_ESTAVEL) break;
        } else {
            estavel = 0;
        }

        if (agora - t0 > TIMEOUT_MOV_MS) {
            Serial.println("[NAV] Timeout ao girar.");
            break;
        }

        float saida = pidGiro.calcular(erro, dt);

        // Piso de giro: vence o atrito estático quando o P sozinho é fraco.
        if (fabsf(erro) > TOL_ANGULO && fabsf(saida) < PWM_MIN_GIRO)
            saida = (erro > 0.0f) ? PWM_MIN_GIRO : -PWM_MIN_GIRO;

        // Desaceleração angular: o teto cai perto do alvo -> chega devagar.
        float teto = (float)VEL_CURVA_MAX;
        if (fabsf(erro) < GRAUS_DECEL)
            teto = VEL_GIRO_MIN + (VEL_CURVA_MAX - VEL_GIRO_MIN) * (fabsf(erro) / GRAUS_DECEL);
        if (saida >  teto) saida =  teto;
        if (saida < -teto) saida = -teto;

        // erro>0 (girar à ESQUERDA/anti-horário): esq p/ trás, dir p/ frente.
        motoresSetCruzeiro(0);
        motoresSetCorrecao((int16_t)saida);
    }

    frearAtivo();
}

void navGirarDireita()   { navGirarDelta(-90.0f); }  // horário  = ângulo diminui
void navGirarEsquerda()  { navGirarDelta(+90.0f); }  // anti-horário = ângulo aumenta
void navGirarMeiaVolta() { navGirarDelta(+180.0f); }

void navDefinirViesCurva(float graus) { VIES_CURVA = graus; }

// Calibração ao vivo da centralização (cascata). Ver navegacao.h.
void navDefinirCentroKp(float v)       { KP_POS = v; }
void navDefinirCentroViesMax(float v)  { VIES_RUMO_MAX = v; }
void navDefinirCentroDeadband(float v) { DEADBAND_POS = v; }

void navParar() {
    motoresParar();
    motoresAtualizar();
}
