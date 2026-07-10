#include <Arduino.h>
#include <WiFi.h>
#include "atuadores/motores.h"
#include "sensores/encoders.h"
#include "sensores/imu.h"
#include "sensores/i2c_bus.h"
#include "sensores/tof.h"
#include "navegacao/pid.h"

// =============================================================================
// BANCADA DA CURVA (Plano A completo) — esquadro + giro seguro + saída centralizada
//
// Objetivo: no MATERIAL do labirinto, garantir que a curva de 90° NUNCA termine
// PRA DENTRO (que é o que faz a roda de dentro raspar). Estratégia:
//   - a navegação continua enxergando 90° cheios (não muda a bússola);
//   - a PRIMITIVA de giro para um TICO ANTES (90 - VIES_CURVA), de propósito;
//   - assim o resíduo cai PRA FORA e a reta seguinte (que mira nos 90° reais)
//     completa os últimos graus vindo do lado seguro.
//
// Não tentamos acertar 90° na mosca (varia com piso/bateria): basta o giro
// parar em ALGUM lugar ANTES de 90 -> o sinal do resíduo é garantidamente fora.
//
// COMO USAR (por nc <IP> 8080 ou captura_log):
//   cd / ce -> MANOBRA COMPLETA (Plano A): esquadro + giro c/ viés + saída centralizada.
//   x       -> só o esquadro (anda até ENCOSTAR na parede) — loga o ToF da aproximação.
//   t       -> monitor de ToF AO VIVO (3 sensores, corrigido+cru) — caracterizar leituras.
//   d / e   -> só o giro puro no lugar (dir/esq), sem esquadro — valida o viés isolado.
//   r       -> repete 8x a ÚLTIMA direção de giro puro (zera a IMU a cada).
//   b / bc  -> DIAGNÓSTICO de deriva do IMU (parado ~8s): b usa o offset atual
//              (compare ANTES/DEPOIS de uma curva/batida); bc recalibra e mede.
//   <num>   -> ajusta VIES_CURVA em graus AO VIVO (ex.: "5" = para 5° antes). Sem regravar.
//   p       -> aborta o movimento em andamento.
//
// LEITURA DO RELATÓRIO: "resíduo" = 90 - |final|.  POSITIVO = parou antes de 90
// = PRA FORA (bom).  NEGATIVO = passou de 90 = PRA DENTRO (ruim, raspa).
//
// TUNING: comece com VIES=0 e mande 'd'/'e' pra MEDIR o overshoot cru (deve dar
// resíduo negativo, ~-5° = pra dentro). Depois suba VIES até o resíduo virar
// positivo com folga (ex.: +2 a +4°) e confirme com 'r' que NENHUMA repetição
// cruza pra dentro.
// =============================================================================

// --- Wi-Fi (DHCP): pega o IP do roteador; leia o IP no serial no boot. --------
const char*    WIFI_SSID       = "Fernando";
const char*    WIFI_PASS       = "12345678";
const uint16_t WIFI_PORTA      = 8080;
const uint32_t WIFI_TIMEOUT_MS = 8000;
WiFiServer server(WIFI_PORTA);
WiFiClient client;

// --- Controle (mesmos parâmetros do teste_giro_stress validado) --------------
const uint16_t INTERVALO_MS   = 10;
const uint32_t TIMEOUT_MS     = 4000;
const float    TOL_ANGULO     = 4.5f;
const float    TOL_VEL_GIRO   = 15.0f;
const uint8_t  CICLOS_ESTAVEL = 3;
const float    VEL_PARADO     = 4.0f;
const uint8_t  CICLOS_PARADO  = 40;

float KP = 5.0f, KI = 8.0f, KD = 0.3f;
const float LIMITE_SAIDA    = 150.0f;
const float LIMITE_INTEGRAL = 15.0f;
const float PWM_MIN_GIRO    = 90.0f;

// --- Desaceleração angular (chega devagar no alvo) ---
const float GRAUS_DECEL  = 40.0f;
const float VEL_GIRO_MIN  = 95.0f;

// --- Freio ativo no fim do giro (mata o rolamento livre) ---
const uint32_t FREIO_MS = 200;

// --- Diagnóstico de deriva do IMU (comandos b / bc) ---
const uint32_t DRIFT_MS = 8000;   // tempo PARADO medindo a deriva de ang_z

// >>> A VARIÁVEL-CHAVE DESTA BANCADA <<<
// Graus que o giro para ANTES dos 90. O resíduo cai pra FORA da curva.
// Ajustável ao vivo mandando um número (ex.: "6.5"). Volta pra este valor no boot.
// VALIDADO no material (2026-07-05): parando COLADINHO na parede (esquadro por
// toque), viés 6 é consistente; 6.5 por segurança. A 1 cm da parede, ~6.5-7.
float VIES_CURVA = 6.5f;

PID pidGiro(KP, KI, KD);

// --- Esquadro por toque (Fase 1 do Plano A): anda até ENCOSTAR na parede -------
// Reusa o PID de rumo e a detecção de stall da reta validada.
float KP_RUMO = 6.0f, KI_RUMO = 1.5f, KD_RUMO = 0.3f;
const float    LIMITE_CORRECAO_RUMO = 70.0f;
const float    LIMITE_INTEGRAL_RUMO = 40.0f;
// PWMs SUAVIZADOS 2026-07-07: bateria cheia + piso liso batiam forte na parede.
// (antes: APROX=100, TOQUE=55, CREEP=120, confirmação a 100)
const int16_t  PWM_APROX      = 90;     // avanço em direção à parede (longe)
const int16_t  PWM_TOQUE      = 45;     // creep suave perto da parede (toque gentil)
const int16_t  PWM_CONFIRM    = 80;     // empurrão de confirmação de toque (< APROX: bate menos)
const uint16_t TOF_CREEP_MM   = 150;    // ToF frontal < isto -> ENTRA em creep (freia mais cedo;
                                        // LATCHA: uma vez em creep, fica até encostar, mesmo que
                                        // o ToF glitche perto da parede -> toque consistente)
const uint32_t ESQUADRO_TIMEOUT_MS = 5000;
// Stall dos encoders = travou. Pode ser a parede OU atrito fraco vencendo o creep;
// por isso, ao suspeitar, empurra a PWM_CONFIRM e só confirma o toque se continuar parado.
const int32_t  STALL_PULSOS_MIN = 3;    // < isto por ciclo = "não avançou"
const uint32_t STALL_MS         = 150;  // parado por este tempo -> SUSPEITA de parede
const uint32_t STALL_CONFIRM_MS = 150;  // ainda parado empurrando -> TOCOU
const int32_t  STALL_ARM_PULSOS = 20;   // só arma o stall (e o creep) depois de andar

PID pidRumo(KP_RUMO, KI_RUMO, KD_RUMO);

// --- Fase 3: sair da curva com centralização EM CASCATA (portado do teste_centralizacao) ---
// O rumo mira nos 90° REAIS (não no ângulo onde o giro parou) -> completa o
// resíduo do viés vindo do lado de fora. A centralização (ToF laterais) só age
// com parede dos DOIS lados; logo após a curva um lado está aberto -> fica off
// até entrar no corredor novo (por isso o esquadro é a defesa principal).
const int16_t  PWM_SAIDA_BASE   = 120;
float          DIST_SAIDA_CM    = 18.0f;   // quanto anda saindo (1 célula = pitch 18)
const int16_t  VEL_MIN_SAIDA    = 50;
const float    DECEL_SAIDA_CM   = 8.0f;
const uint32_t TIMEOUT_SAIDA_MS = 8000;
// Centralização em CASCATA (validada no teste_centralizacao): erro de posição ->
// VIÉS no setpoint de rumo (não mais um ajuste diferencial). Gate por dist. CORRIGIDA.
const float    DIF_ALVO       = -55.0f;    // (esq_corr - dir_cru) medido no CENTRO (~-60)
const float    KP_POS         = 0.10f;     // graus de viés de rumo por unidade de difC
const float    VIES_RUMO_MAX  = 7.0f;      // teto do viés de rumo (graus)
const float    DEADBAND_POS   = 10.0f;     // zona morta em unidades de difC (~6 mm), contínua
const uint16_t LIMIAR_PAREDE  = 140;       // mm CORRIGIDOS: abaixo disso = tem parede (era cru 300)
const uint16_t TOF_MIN_VALIDO = 1;         // sanidade ToF: rejeita 0
const uint16_t TOF_MAX_VALIDO = 2000;      // sanidade ToF: rejeita 8191/lixo/aberto longe

static inline bool tofValido(uint16_t v) { return v >= TOF_MIN_VALIDO && v <= TOF_MAX_VALIDO; }

struct Amostra {
    uint32_t t_ms;
    uint8_t  fase;                     // 2 = giro, 3 = saída (esquadro não é logado)
    float    ang_z, erro, velGiro, correcao;
    int16_t  pwmEsq, pwmDir;
    int32_t  encEsq, encDir;
    uint16_t esq_mm, dir_mm;           // ToF laterais (só fase 3; 0 no giro)
    float    viesRumo;                 // viés de rumo da centralização (graus; fases 1 e 3)
};
const int MAX_AMOSTRAS = 900;          // cobre giro + saída na mesma manobra
Amostra buffer[MAX_AMOSTRAS];
int      nAmostras = 0;
uint32_t t0Global  = 0;

struct Resultado {
    float       alvo, final_, erro;
    const char* motivo;
    float       dtMax;
    uint16_t    perdidos;
};

char ultimaDir = 'd';   // pra o comando 'r' (repete a última direção)

// Resultado do esquadro (Fase 1).
struct Esquadro {
    bool        tocou;      // encostou na parede (stall) vs timeout/abort
    float       distCm;     // quanto andou até parar
    float       rumoFinal;  // rumo ao encostar (deve ficar ~0 = esquadrado)
    const char* motivo;
};

// --- Protótipos ---
void      conectarWiFi();
bool      lerComando(char& c);
void      logLinha(const String& s);
Resultado girar(float alvoAbs);
void      frear();
void      despejarBuffer(const char* nome);
void      relatar(char dir, const Resultado& r);
void      umaCurva(char dir, bool comCSV);
Esquadro  esquadro();
void      umaCurvaCompleta(char dir);
void      sairComCentralizacao(char dir, float distCm);
void      driftTest(bool recalibrar);
void      monitorToF();

void setup() {
    Serial.begin(115200);
    delay(200);

    motoresInit();
    encodersInit();
    i2cInit();                       // Wire nos pinos certos (IMU + ToF usam)
    if (!imuInit()) {
        Serial.println("[CURVA] ERRO: IMU nao inicializou. Travado.");
        while (true) delay(1000);
    }
    if (!tofInit()) {
        Serial.println("[CURVA] ERRO: ToF nao inicializou. Travado.");
        while (true) delay(1000);
    }

    pidGiro.definirGanhos(KP, KI, KD);
    pidGiro.definirLimiteSaida(-LIMITE_SAIDA, LIMITE_SAIDA);
    pidGiro.definirLimiteIntegral(LIMITE_INTEGRAL);

    pidRumo.definirGanhos(KP_RUMO, KI_RUMO, KD_RUMO);
    pidRumo.definirLimiteSaida(-LIMITE_CORRECAO_RUMO, LIMITE_CORRECAO_RUMO);
    pidRumo.definirLimiteIntegral(LIMITE_INTEGRAL_RUMO);

    Serial.println("[CURVA] Calibrando IMU — mantenha o robo PARADO...");
    imuCalibrarOffsetZ(300);

    conectarWiFi();
    logLinha("[CURVA] Pronto. cd/ce=manobra x=esquadro d/e=giro t=ToF b/bc=drift r=repete8x <num>=viés p=aborta");
    logLinha(String("[CURVA] VIES_CURVA = ") + String(VIES_CURVA, 1) + " graus");
}

void loop() {
    if (server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            logLinha("=== Conectado a bancada de curva (Plano A) ===");
            logLinha("cd/ce=manobra | x=esquadro | d/e=giro | t=ToF ao vivo | b/bc=drift IMU | r=repete8x | <num>=viés | p=aborta");
            logLinha(String("VIES_CURVA atual = ") + String(VIES_CURVA, 1) + " graus");
        } else {
            server.available().stop();
        }
    }

    // Parse por LINHA (permite mandar um número pra ajustar o VIES ao vivo).
    static String bufCmd = "";
    char c;
    while (lerComando(c)) {
        if (c != '\n' && c != '\r') {
            if (bufCmd.length() < 12) bufCmd += c;
            continue;
        }
        bufCmd.trim();
        if (bufCmd.length() == 0) { continue; }

        if (bufCmd == "cd" || bufCmd == "CD") {
            umaCurvaCompleta('d');
        } else if (bufCmd == "ce" || bufCmd == "CE") {
            umaCurvaCompleta('e');
        } else if (bufCmd == "x" || bufCmd == "X") {
            logLinha("[CURVA] Esquadro isolado (anda ate encostar; loga ToF da aproximacao)...");
            nAmostras = 0; t0Global = millis();
            Esquadro es = esquadro();
            logLinha(String("[CURVA] esquadro: andou ") + String(es.distCm, 1)
                     + " cm, " + es.motivo + ", rumo final=" + String(es.rumoFinal, 2) + " deg");
            despejarBuffer("esquadro");
        } else if (bufCmd == "t" || bufCmd == "T") {
            monitorToF();
        } else if (bufCmd == "b" || bufCmd == "B") {
            driftTest(false);   // deriva com o offset ATUAL (pra comparar antes/depois)
        } else if (bufCmd == "bc" || bufCmd == "BC") {
            driftTest(true);    // recalibra o offset e mede a deriva
        } else if (bufCmd == "d" || bufCmd == "D") {
            umaCurva('d', true);
        } else if (bufCmd == "e" || bufCmd == "E") {
            umaCurva('e', true);
        } else if (bufCmd == "r" || bufCmd == "R") {
            logLinha(String("[CURVA] Repetindo 8x a direcao '") + ultimaDir + "'...");
            int dentro = 0;
            for (int i = 0; i < 8; i++) {
                umaCurva(ultimaDir, false);
                imuAtualizar();
                if (fabsf(imuLerAnguloZ()) > 90.0f) dentro++;   // cruzou pra dentro
                delay(300);
            }
            logLinha(String("[CURVA] Fim das 8 repeticoes. Cruzaram PRA DENTRO: ")
                     + dentro + "/8 " + (dentro == 0 ? "(OK)" : "(RUIM)"));
        } else {
            const float v = bufCmd.toFloat();
            // aceita "0" tambem (toFloat de "0" = 0, entao trata digito explicito)
            if (v > 0.0f || bufCmd == "0") {
                VIES_CURVA = v;
                logLinha(String("[CURVA] VIES_CURVA ajustado para ") + String(VIES_CURVA, 1) + " graus");
            }
        }
        bufCmd = "";
    }

    delay(5);
}

// Executa UMA curva na direcao dir ('d'/'e'), aplicando o VIES. Loga o relatorio
// e, se comCSV, despeja a telemetria.
void umaCurva(char dir, bool comCSV) {
    ultimaDir = dir;
    const float mag   = 90.0f - VIES_CURVA;                 // para ANTES de 90
    const float alvo  = (dir == 'd') ? -mag : +mag;          // IMU: -90 = direita

    imuZerarAnguloZ(); encodersZerar(); nAmostras = 0; t0Global = millis();
    Resultado r = girar(alvo);
    relatar(dir, r);
    if (comCSV) despejarBuffer(dir == 'd' ? "curva_dir" : "curva_esq");
}

// -----------------------------------------------------------------------------
// FASE 1 — Esquadro por toque: anda pra frente com heading-hold (IMU), desacelera
// perto da parede (ToF frontal) e para quando ENCOSTA (stall dos encoders). Isso
// põe o eixo no melhor caso (offset mínimo) E esquadra o rumo contra a parede.
// -----------------------------------------------------------------------------
Esquadro esquadro() {
    encodersZerar();
    imuZerarAnguloZ();
    pidRumo.resetar();

    const uint32_t t0 = millis();
    uint32_t ultimo   = t0;
    int32_t  encAnterior   = 0;
    uint32_t ultimoAvanco  = t0;
    bool     emCreep       = false;   // latch: uma vez perto, fica devagar até encostar
    bool     suspeito      = false;   // stall candidato -> empurra a 100 pra confirmar
    uint32_t suspeitoDesde = 0;

    Esquadro es; es.tocou = false; es.motivo = "timeout"; es.distCm = 0; es.rumoFinal = 0;

    while (true) {
        imuAtualizar();
        motoresAtualizar();

        const uint32_t agora = millis();
        if (agora - ultimo < INTERVALO_MS) { delay(1); continue; }
        const float dt = (agora - ultimo) / 1000.0f;
        ultimo = agora;

        char cmd;
        if (lerComando(cmd) && (cmd == 'p' || cmd == 'P')) { es.motivo = "abortado"; break; }

        const int32_t eEsq = encoderLerEsquerdo();
        const int32_t eDir = encoderLerDireito();
        const float   ang  = imuLerAnguloZ();

        // Stall = travou. Pode ser a PAREDE ou o atrito vencendo o creep fraco.
        // Ao suspeitar, empurra a 100 (abaixo); se continuar parado, é a parede.
        const int32_t encSoma  = eEsq + eDir;
        const int32_t encDelta = encSoma > encAnterior ? encSoma - encAnterior
                                                       : encAnterior - encSoma;
        if (encDelta >= STALL_PULSOS_MIN) {
            encAnterior  = encSoma;
            ultimoAvanco = agora;
            suspeito     = false;                 // andou -> não era a parede
        } else if (encSoma > STALL_ARM_PULSOS && agora - ultimoAvanco > STALL_MS) {
            if (!suspeito) { suspeito = true; suspeitoDesde = agora; }
            if (agora - suspeitoDesde > STALL_CONFIRM_MS) {
                es.tocou = true; es.motivo = "tocou"; break;
            }
        }

        if (agora - t0 > ESQUADRO_TIMEOUT_MS) { es.motivo = "timeout"; break; }

        // Centralização em CASCATA na APROXIMAÇÃO: se começar torto, o viés no
        // setpoint de rumo (baseline 0) puxa pro meio antes de chegar na parede ->
        // chega centrado pra virar. Deadband evita cutuco perto do centro. Gate por
        // parede dos 2 lados (dist. CORRIGIDA) + sanidade ToF.
        const uint16_t dEsq     = tofLerDistancia(1);        // esq corrigida (sinal)
        const uint16_t dDir     = tofLerDistanciaBruta(2);   // dir CRUA (sinal)
        const uint16_t dDirCorr = tofLerDistancia(2);        // dir corrigida (só p/ gatear)
        float viesRumo = 0.0f;
        if (tofValido(dEsq) && dEsq < LIMIAR_PAREDE &&
            tofValido(dDirCorr) && dDirCorr < LIMIAR_PAREDE && tofValido(dDir)) {
            const float erroPos = ((float)dEsq - (float)dDir) - DIF_ALVO;
            const float erroEff = fabsf(erroPos) <= DEADBAND_POS ? 0.0f
                                  : erroPos - (erroPos > 0.0f ? DEADBAND_POS : -DEADBAND_POS);
            viesRumo = KP_POS * erroEff;
            if (viesRumo >  VIES_RUMO_MAX) viesRumo =  VIES_RUMO_MAX;
            if (viesRumo < -VIES_RUMO_MAX) viesRumo = -VIES_RUMO_MAX;
        }
        const float correcao = pidRumo.calcular(viesRumo - ang, dt);

        // Velocidade: arranca a PWM_APROX e só entra em creep DEPOIS de ter andado
        // (garante o breakaway mesmo começando colado). Latch. No stall suspeito,
        // empurra a PWM_CONFIRM (< APROX: confirma sem bater forte).
        const uint16_t df = tofLerDistancia(0);           // frontal (com correção)
        if (df > 0 && df < TOF_CREEP_MM && encSoma > STALL_ARM_PULSOS) emCreep = true;
        int16_t vel = emCreep ? PWM_TOQUE : PWM_APROX;
        if (suspeito) vel = PWM_CONFIRM;                  // empurrão de confirmação (suave)

        motoresSetCruzeiro(vel);
        motoresSetCorrecao((int16_t)correcao);

        // Log da APROXIMAÇÃO (fase 1) — mostra o que os ToF leem e como a
        // centralização age ANTES de virar (ToF já lido acima, custo zero).
        if (nAmostras < MAX_AMOSTRAS) {
            Amostra& a = buffer[nAmostras++];
            a.t_ms     = agora - t0Global;   a.fase = 1;
            a.ang_z    = ang;   a.erro = viesRumo - ang;   a.velGiro = 0.0f;   a.correcao = correcao;
            a.pwmEsq   = motorLerVelocidadeAtual(MOTOR_ESQUERDO);
            a.pwmDir   = motorLerVelocidadeAtual(MOTOR_DIREITO);
            a.encEsq   = eEsq;   a.encDir = eDir;
            a.esq_mm   = dEsq;   a.dir_mm = dDir;   a.viesRumo = viesRumo;
        }
    }

    frear();
    es.distCm    = 0.5f * (encoderDistanciaEsquerdaCm() + encoderDistanciaDireitaCm());
    es.rumoFinal = imuLerAnguloZ();
    return es;
}

// Manobra completa do Plano A, num CSV só (coluna 'fase'):
//   Fase 1 esquadro por toque -> Fase 2 giro com viés (cai pra fora) ->
//   Fase 3 sai com centralização (rumo mira nos 90° REAIS, completa o resíduo).
// Só segue se REALMENTE encostou (senão a referência de posição/rumo é inválida).
void umaCurvaCompleta(char dir) {
    nAmostras = 0; t0Global = millis();    // buffer único pra manobra inteira

    // --- Fase 1: esquadro por toque (não logado no buffer; só resumo) ---
    logLinha("[CURVA] Fase 1: esquadro por toque (andando ate a parede)...");
    Esquadro es = esquadro();
    logLinha(String("[CURVA] esquadro: andou ") + String(es.distCm, 1)
             + " cm, " + es.motivo + ", rumo final=" + String(es.rumoFinal, 2) + " deg");
    if (!es.tocou) {
        logLinha("[CURVA] NAO encostou (timeout/abort) -> manobra CANCELADA.");
        return;
    }

    // --- Fase 2: giro com viés (cai pra fora) ---
    delay(200);   // assenta antes de girar
    logLinha("[CURVA] Fase 2: giro com vies (cai pra fora)...");
    const float mag  = 90.0f - VIES_CURVA;
    const float alvo = (dir == 'd') ? -mag : +mag;      // IMU: -90 = direita
    imuZerarAnguloZ(); encodersZerar();
    Resultado r = girar(alvo);
    relatar(dir, r);

    // --- Fase 3: sair com centralização (rumo mira nos 90° reais) ---
    delay(200);
    logLinha("[CURVA] Fase 3: saindo com centralizacao...");
    sairComCentralizacao(dir, DIST_SAIDA_CM);

    despejarBuffer(dir == 'd' ? "curva_full_dir" : "curva_full_esq");
}

// -----------------------------------------------------------------------------
// FASE 3 — Sai da curva andando reto (heading-hold nos 90° REAIS, o que completa
// o resíduo do viés) e, quando há parede dos DOIS lados, centraliza pelos ToF.
// NÃO zera a IMU: mantém o referencial do giro, onde reto-pra-frente = ±90.
// -----------------------------------------------------------------------------
void sairComCentralizacao(char dir, float distCm) {
    encodersZerar();
    pidRumo.resetar();
    const float alvoRumo = (dir == 'd') ? -90.0f : +90.0f;   // rumo VERDADEIRO

    const uint32_t t0 = millis();
    uint32_t ultimo   = t0;
    int32_t  encAnterior  = 0;
    uint32_t ultimoAvanco = t0;
    bool     centralizou  = false;

    while (true) {
        imuAtualizar();
        motoresAtualizar();

        const uint32_t agora = millis();
        if (agora - ultimo < INTERVALO_MS) { delay(1); continue; }
        const float dt = (agora - ultimo) / 1000.0f;
        ultimo = agora;

        char cmd;
        if (lerComando(cmd) && (cmd == 'p' || cmd == 'P')) break;

        const int32_t eEsq = encoderLerEsquerdo();
        const int32_t eDir = encoderLerDireito();
        const float   ang  = imuLerAnguloZ();
        const float   distMedia = 0.5f * (encoderDistanciaEsquerdaCm() + encoderDistanciaDireitaCm());

        if (distMedia >= distCm) break;
        if (agora - t0 > TIMEOUT_SAIDA_MS) { logLinha("[CURVA] saida: TIMEOUT."); break; }

        const int32_t encSoma  = eEsq + eDir;
        const int32_t encDelta = encSoma > encAnterior ? encSoma - encAnterior
                                                       : encAnterior - encSoma;
        if (encDelta >= STALL_PULSOS_MIN) {
            encAnterior = encSoma; ultimoAvanco = agora;
        } else if (encSoma > STALL_ARM_PULSOS && agora - ultimoAvanco > STALL_MS) {
            logLinha("[CURVA] saida: STALL (encoders parados)."); break;
        }

        // Centralização em CASCATA: viés no setpoint de rumo. Só com parede dos DOIS
        // lados (dist. CORRIGIDA) e leitura válida. Usa o CRU da direita como sinal.
        const uint16_t dEsq     = tofLerDistancia(1);
        const uint16_t dDir     = tofLerDistanciaBruta(2);
        const uint16_t dDirCorr = tofLerDistancia(2);
        float viesRumo = 0.0f;
        if (tofValido(dEsq) && dEsq < LIMIAR_PAREDE &&
            tofValido(dDirCorr) && dDirCorr < LIMIAR_PAREDE && tofValido(dDir)) {
            centralizou = true;
            const float erroPos = ((float)dEsq - (float)dDir) - DIF_ALVO;
            const float erroEff = fabsf(erroPos) <= DEADBAND_POS ? 0.0f
                                  : erroPos - (erroPos > 0.0f ? DEADBAND_POS : -DEADBAND_POS);
            viesRumo = KP_POS * erroEff;
            if (viesRumo >  VIES_RUMO_MAX) viesRumo =  VIES_RUMO_MAX;
            if (viesRumo < -VIES_RUMO_MAX) viesRumo = -VIES_RUMO_MAX;
        }

        // Rumo: completa o resíduo do giro (mira nos 90° reais) + viés de centralização.
        const float erroRumo = (alvoRumo + viesRumo) - ang;
        const float correcao = pidRumo.calcular(erroRumo, dt);
        const int16_t total = (int16_t)correcao;

        int16_t vel = PWM_SAIDA_BASE;
        const float restante = distCm - distMedia;
        if (restante < DECEL_SAIDA_CM) {
            vel = VEL_MIN_SAIDA + (int16_t)((PWM_SAIDA_BASE - VEL_MIN_SAIDA) * (restante / DECEL_SAIDA_CM));
            if (vel < VEL_MIN_SAIDA) vel = VEL_MIN_SAIDA;
        }
        motoresSetCruzeiro(vel);
        motoresSetCorrecao(total);

        if (nAmostras < MAX_AMOSTRAS) {
            Amostra& a = buffer[nAmostras++];
            a.t_ms     = agora - t0Global;   a.fase = 3;
            a.ang_z    = ang;   a.erro = erroRumo;   a.velGiro = imuLerGiroZ();   a.correcao = correcao;
            a.pwmEsq   = motorLerVelocidadeAtual(MOTOR_ESQUERDO);
            a.pwmDir   = motorLerVelocidadeAtual(MOTOR_DIREITO);
            a.encEsq   = eEsq;   a.encDir = eDir;
            a.esq_mm   = dEsq;   a.dir_mm = dDir;   a.viesRumo = viesRumo;
        }
    }

    motoresFrear();
    const uint32_t tf = millis();
    while (millis() - tf < FREIO_MS) { imuAtualizar(); delay(2); }
    motoresParar();
    motoresAtualizar();

    const float distFinal = 0.5f * (encoderDistanciaEsquerdaCm() + encoderDistanciaDireitaCm());
    logLinha(String("[CURVA] saida: andou ") + String(distFinal, 1)
             + " cm, rumo final=" + String(imuLerAnguloZ(), 2) + " deg (alvo " + String(alvoRumo, 0)
             + "), centralizou=" + (centralizou ? "sim" : "nao (corredor aberto)"));
}

// -----------------------------------------------------------------------------
// Gira ate o angulo absoluto alvoAbs. (Ported do teste_giro_stress.)
// -----------------------------------------------------------------------------
Resultado girar(float alvoAbs) {
    pidGiro.resetar();
    const uint32_t t0 = millis();
    uint32_t ultimo   = t0;
    uint8_t  estavel  = 0, parado = 0;
    bool     arrancou = false;

    Resultado res; res.alvo = alvoAbs; res.motivo = "alvo"; res.dtMax = 0; res.perdidos = 0;

    while (true) {
        imuAtualizar();
        motoresAtualizar();

        const uint32_t agora = millis();
        if (agora - ultimo < INTERVALO_MS) { delay(1); continue; }
        const float dt = (agora - ultimo) / 1000.0f;
        ultimo = agora;
        if (dt > res.dtMax) res.dtMax = dt;
        if (dt > 0.2f)      res.perdidos++;

        char cmd;
        if (lerComando(cmd) && (cmd == 'p' || cmd == 'P')) { res.motivo = "abortado"; break; }

        const float ang     = imuLerAnguloZ();
        const float velGiro = imuLerGiroZ();
        const float erro    = alvoAbs - ang;

        if (fabsf(erro) <= TOL_ANGULO && fabsf(velGiro) <= TOL_VEL_GIRO) {
            if (++estavel >= CICLOS_ESTAVEL) { res.motivo = "ok"; break; }
        } else {
            estavel = 0;
        }

        if (fabsf(velGiro) > VEL_PARADO) arrancou = true;
        if (arrancou && fabsf(velGiro) < VEL_PARADO && fabsf(erro) > TOL_ANGULO) {
            if (++parado >= CICLOS_PARADO) { res.motivo = "travou"; break; }
        } else {
            parado = 0;
        }

        if (agora - t0 > TIMEOUT_MS) { res.motivo = "timeout"; break; }

        float saida = pidGiro.calcular(erro, dt);
        if (fabsf(erro) > TOL_ANGULO && fabsf(saida) < PWM_MIN_GIRO)
            saida = (erro > 0.0f) ? PWM_MIN_GIRO : -PWM_MIN_GIRO;
        float teto = LIMITE_SAIDA;
        if (fabsf(erro) < GRAUS_DECEL)
            teto = VEL_GIRO_MIN + (LIMITE_SAIDA - VEL_GIRO_MIN) * (fabsf(erro) / GRAUS_DECEL);
        if (saida >  teto) saida =  teto;
        if (saida < -teto) saida = -teto;
        motoresSetCruzeiro(0);
        motoresSetCorrecao((int16_t)saida);

        if (nAmostras < MAX_AMOSTRAS) {
            Amostra& a = buffer[nAmostras++];
            a.t_ms     = agora - t0Global;   a.fase = 2;
            a.ang_z    = ang;   a.erro = erro;   a.velGiro = velGiro;   a.correcao = saida;
            a.pwmEsq   = motorLerVelocidadeAtual(MOTOR_ESQUERDO);
            a.pwmDir   = motorLerVelocidadeAtual(MOTOR_DIREITO);
            a.encEsq   = encoderLerEsquerdo();  a.encDir = encoderLerDireito();
            a.esq_mm   = 0;  a.dir_mm = 0;  a.viesRumo = 0.0f;
        }
    }

    frear();
    res.final_ = imuLerAnguloZ();
    res.erro   = alvoAbs - res.final_;
    return res;
}

// Freio ATIVO (short-brake) + espera assentar. Mede o final DEPOIS disto.
void frear() {
    motoresFrear();
    const uint32_t tf = millis();
    while (millis() - tf < FREIO_MS) {
        imuAtualizar();
        delay(2);
    }
    motoresParar();
    motoresAtualizar();
}

// Relatorio legivel: diz se caiu PRA FORA (bom) ou PRA DENTRO (raspa).
void relatar(char dir, const Resultado& r) {
    const float nominal = (dir == 'd') ? -90.0f : +90.0f;
    const float residuo = 90.0f - fabsf(r.final_);   // >0 = parou antes de 90 = PRA FORA
    const char* lado    = (residuo >= 0.0f) ? "PRA FORA (bom)" : "PRA DENTRO (RUIM, raspa)";
    logLinha(String("[CURVA] dir=") + dir
             + " nominal=" + String(nominal, 0)
             + " alvoReal=" + String(r.alvo, 1)
             + " final=" + String(r.final_, 2)
             + " | residuo=" + String(residuo, 2) + " -> " + lado
             + " | fim=" + r.motivo);
}

// -----------------------------------------------------------------------------
// DIAGNÓSTICO — monitor de ToF AO VIVO. Mostra os 3 sensores em CORRIGIDO e CRU
// pra você posicionar o robô (centrado / colado esq / colado dir / na junção) e
// ver o que cada um lê. A última coluna é o 'dif' que a centralização usa
// (esq_corrigido - dir_CRU), com alvo DIF_ALVO=-55 no centro.
// -----------------------------------------------------------------------------
void monitorToF() {
    logLinha("[TOF] Monitor ao vivo (mm). Mova o robo; 'p' pra sair.");
    logLinha("[TOF] esq corr/cru | dir corr/cru | frente corr/cru | dif(esq_corr - dir_cru)");
    while (true) {
        char c;
        if (lerComando(c) && (c == 'p' || c == 'P')) break;

        const uint16_t ec = tofLerDistancia(1), er = tofLerDistanciaBruta(1);   // esquerda
        const uint16_t dc = tofLerDistancia(2), dr = tofLerDistanciaBruta(2);   // direita
        const uint16_t fc = tofLerDistancia(0), fr = tofLerDistanciaBruta(0);   // frente

        char linha[128];
        snprintf(linha, sizeof(linha),
                 "[TOF] esq %4u/%4u | dir %4u/%4u | fre %4u/%4u | dif %d",
                 ec, er, dc, dr, fc, fr, (int)ec - (int)dr);
        logLinha(String(linha));
        delay(200);
    }
    logLinha("[TOF] Monitor encerrado.");
}

// -----------------------------------------------------------------------------
// DIAGNÓSTICO — mede a DERIVA do ang_z com o robô PARADO (motores off). Integra o
// giro Z como na operação real; se ang_z sobe/desce em rampa = bias residual.
//   recalibrar=false (b)  -> usa o offset ATUAL (rode antes/depois de curva/batida)
//   recalibrar=true  (bc) -> recalibra o offset primeiro (vê se "cura" a deriva)
// -----------------------------------------------------------------------------
void driftTest(bool recalibrar) {
    motoresParar();
    motoresAtualizar();
    delay(100);

    if (recalibrar) {
        logLinha("[DRIFT] Recalibrando offset Z — mantenha PARADO...");
        imuCalibrarOffsetZ(300);
    }
    imuZerarAnguloZ();

    logLinha("---- INICIO TELEMETRIA (CSV) [drift] ----");
    logLinha("t_ms,ang_z,gyroZ");
    const uint32_t t0 = millis();
    uint32_t ultimoLog = t0;
    char linha[80];
    while (millis() - t0 < DRIFT_MS) {
        imuAtualizar();                       // integra ang_z como na operação real

        char c;
        if (lerComando(c) && (c == 'p' || c == 'P')) break;

        const uint32_t agora = millis();
        if (agora - ultimoLog >= 100) {       // amostra lenta: a deriva é lenta
            ultimoLog = agora;
            const float ang = imuLerAnguloZ();
            const float gz  = imuLerGiroZ();  // taxa instantânea (offset já subtraído)
            snprintf(linha, sizeof(linha), "%lu,%.3f,%.3f",
                     (unsigned long)(agora - t0), ang, gz);
            logLinha(String(linha));
        }
        delay(2);
    }
    logLinha("---- FIM (drift) ----");

    const float angFim = imuLerAnguloZ();
    const float taxa   = angFim / (DRIFT_MS / 1000.0f);
    logLinha(String("[DRIFT] Parado ") + String(DRIFT_MS / 1000.0f, 0) + "s: ang_z -> "
             + String(angFim, 2) + " deg  (deriva ~" + String(taxa, 3) + " deg/s)  "
             + (fabsf(taxa) < 0.3f ? "OK, estavel" : "DERIVA! bias residual"));
}

void despejarBuffer(const char* nome) {
    logLinha(String("---- INICIO TELEMETRIA (CSV) [") + nome + "] ----");
    logLinha("t_ms,fase,ang_z,erro,velGiro,correcao,pwmEsq,pwmDir,encEsq,encDir,esq_mm,dir_mm,viesRumo");
    char linha[160];
    for (int i = 0; i < nAmostras; i++) {
        const Amostra& a = buffer[i];
        snprintf(linha, sizeof(linha), "%lu,%u,%.2f,%.2f,%.2f,%.2f,%d,%d,%ld,%ld,%u,%u,%.2f",
                 (unsigned long)a.t_ms, a.fase, a.ang_z, a.erro, a.velGiro, a.correcao,
                 a.pwmEsq, a.pwmDir, (long)a.encEsq, (long)a.encDir,
                 a.esq_mm, a.dir_mm, a.viesRumo);
        logLinha(String(linha));
    }
    logLinha(String("---- FIM (") + String(nAmostras) + " amostras) ----");
}

bool lerComando(char& c) {
    if (Serial.available() > 0) { c = (char)Serial.read(); return true; }
    if (client && client.connected() && client.available() > 0) {
        c = (char)client.read();
        return true;
    }
    return false;
}

void logLinha(const String& s) {
    Serial.println(s);
    if (client && client.connected()) client.println(s);
}

void conectarWiFi() {
    Serial.printf("[CURVA] Conectando em \"%s\" (DHCP) ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[CURVA] Wi-Fi OK. Conecte em  %s:%u\n",
                      WiFi.localIP().toString().c_str(), WIFI_PORTA);
        server.begin();
    } else {
        Serial.println("\n[CURVA] Wi-Fi FALHOU. Cheque SSID/senha e se a rede e 2,4GHz.");
    }
}
