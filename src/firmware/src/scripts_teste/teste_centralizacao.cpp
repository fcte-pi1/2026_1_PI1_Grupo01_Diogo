#include <Arduino.h>
#include <WiFi.h>
#include "atuadores/motores.h"
#include "sensores/encoders.h"
#include "sensores/imu.h"
#include "sensores/i2c_bus.h"
#include "sensores/tof.h"
#include "navegacao/pid.h"

// =============================================================================
// BANCADA DE CENTRALIZAÇÃO EM CASCATA (posição lateral -> setpoint de rumo)
//
// PROBLEMA que este bench resolve (visto nos logs de 2026-07-07):
//   A centralização antiga somava um ajuste DIFERENCIAL (±PWM) por cima de um
//   heading-hold STIFF. O PID de rumo (que segura ang=0) CANCELAVA a guinada do
//   centralizador -> o robô quase não transladava de lado (ficava preso no
//   deslocamento inicial). Você corrige rumo OU translada; um viés diferencial
//   puro é anulado pelo laço de rumo.
//
// IDEIA NOVA (cascata / dois laços aninhados):
//   - Laço EXTERNO (posição): erroPos = difC - DIF_ALVO. Gera um VIÉS no ALVO
//     de rumo (aponta o nariz um pouco pro lado que precisa ir).
//   - Laço INTERNO (rumo): o PID de rumo já validado PERSEGUE esse alvo enviesado.
//   Agora o rumo é ALIADO da centralização: ele EXECUTA a translação em vez de
//   brigar. Quando o robô chega ao centro, erroPos->0, o viés some e o nariz
//   volta a ficar paralelo. É um controlador P de posição lateral tendo o RUMO
//   como atuador (estável, converge sozinho).
//
//   Sinais (conferidos na caracterização de 07/07):
//     difC = esq_corr - dir_cru.  Centro real ~ -60 (DIF_ALVO=-55).
//     Colado à ESQUERDA -> difC ~ -111 -> erroPos < 0 -> viés < 0 -> nariz p/
//       DIREITA (IMU diminui) -> afasta da esquerda. OK.
//     Colado à DIREITA  -> difC ~ +15  -> erroPos > 0 -> viés > 0 -> nariz p/
//       ESQUERDA (IMU aumenta) -> afasta da direita. OK.
//
// TAMBÉM CORRIGE (bugs achados nos logs):
//   - Limiar de "tem parede" agora pela distância CORRIGIDA (não o cru 300, que
//     ligava contra a junção ABERTA lida ~210). Parede real dá corr < ~140.
//   - Filtro de sanidade do ToF: rejeita 0 / 8191 / >2000 (o glitch esporádico).
//   - Stall POR-RODA: pega a roda travada (o detector antigo somava os dois
//     encoders; a roda livre mascarava o travamento -> "travou na parede").
//
// COMO VALIDAR: comece o robô DE PROPÓSITO encostado/deslocado num lado de um
// corredor com parede dos DOIS lados e mande a distância. Ele deve VOLTAR PRO
// MEIO (difC -> DIF_ALVO) ao longo do caminho. O CSV loga difC vs distância.
//
// COMANDOS (nc <IP> 8080 ou captura_log):
//   <num>       -> distância em cm e dispara (ex.: 40).
//   s           -> repete a última distância.
//   kp <num>    -> ajusta KP_POS ao vivo (ganho posição->rumo). Ex.: "kp 0.25".
//   vm <num>    -> ajusta o teto do viés de rumo em graus. Ex.: "vm 12".
//   t           -> monitor de ToF ao vivo (corrigido+cru dos 3) pra reposicionar.
//   p           -> aborta o movimento em andamento.
// =============================================================================

// --- Wi-Fi (DHCP): pega o IP do roteador; leia o IP no serial no boot. --------
const char*    WIFI_SSID       = "Vilbs";
const char*    WIFI_PASS       = "12345678";
const uint16_t WIFI_PORTA      = 8080;
const uint32_t WIFI_TIMEOUT_MS = 8000;
WiFiServer server(WIFI_PORTA);
WiFiClient client;

// --- Parâmetros do movimento ---
float          DISTANCIA_ALVO_CM = 40.0f;   // mutável pelo terminal
const int16_t  PWM_BASE          = 100;     // cruzeiro (labirinto anda ~90-100)
const uint16_t INTERVALO_MS      = 10;
const uint32_t TIMEOUT_MS        = 10000;

// --- Desaceleração + freio ativo (portado da reta validada) ---
const int16_t  VEL_MIN   = 65;              // subido 50->65 (atrito do labirinto)
const float    DECEL_CM  = 8.0f;
const uint32_t FREIO_MS  = 250;

// --- PID de RUMO (heading-hold) — ganhos validados no teste_reta_imu ---
float KP = 6.0f, KI = 1.5f, KD = 0.3f;
const float LIMITE_CORRECAO = 70.0f;
const float LIMITE_INTEGRAL = 40.0f;
PID pidRumo(KP, KI, KD);

// --- Centralização em CASCATA (posição lateral -> viés no setpoint de rumo) ---
// Valores VALIDADOS 2026-07-07: 0.10/7 derrubou o serpenteio pela metade
// (max|ang| 15,8°->6,8°) mantendo convergência. Deadband mata a micro-serpenteada
// do finalzinho (para de cutucar dentro de ~±6 mm do centro).
const float    DIF_ALVO        = -55.0f;    // (esq_corr - dir_cru) no CENTRO (medido ~-60)
float          KP_POS          = 0.10f;     // graus de viés de rumo por unidade de difC
float          VIES_RUMO_MAX   = 7.0f;      // teto do viés de rumo (graus) — corredor é estreito
float          DEADBAND_POS    = 10.0f;     // zona morta em unidades de difC (~6 mm): sem cutuco
const uint16_t LIMIAR_PAREDE   = 140;       // mm CORRIGIDOS: abaixo disso = tem parede
const uint16_t TOF_MIN_VALIDO  = 1;         // sanidade: rejeita 0
const uint16_t TOF_MAX_VALIDO  = 2000;      // sanidade: rejeita 8191/lixo/aberto longe

// --- Buffer de telemetria (em RAM) ---
struct Amostra {
    uint32_t t_ms;
    int32_t  encEsq, encDir;
    float    ang_z;         // rumo atual
    float    alvoRumo;      // setpoint de rumo (= viés da centralização)
    float    difC;          // esq_corr - dir_cru
    float    erroPos;       // difC - DIF_ALVO
    float    correcao;      // saída do PID de rumo
    uint16_t esq_mm;        // esq corrigido
    uint16_t dir_mm;        // dir cru (o que a centralização usa)
    uint8_t  gate;          // 1 = centralização ativa (parede dos 2 lados)
    int16_t  pwmEsq, pwmDir;
};
const int MAX_AMOSTRAS = 700;
Amostra buffer[MAX_AMOSTRAS];
int nAmostras = 0;

// --- Protótipos ---
void conectarWiFi();
void executarReta();
void despejarBuffer();
void monitorToF();
void logLinha(const String& s);
bool lerComando(char& c);

static inline bool tofValido(uint16_t v) {
    return v >= TOF_MIN_VALIDO && v <= TOF_MAX_VALIDO;
}

void setup() {
    Serial.begin(115200);
    delay(200);

    motoresInit();
    encodersInit();
    i2cInit();                       // Wire nos pinos certos (IMU + ToF usam)

    if (!imuInit()) {
        Serial.println("[CENTRO2] ERRO: IMU nao inicializou. Travado.");
        while (true) delay(1000);
    }
    if (!tofInit()) {
        Serial.println("[CENTRO2] ERRO: ToF nao inicializou. Travado.");
        while (true) delay(1000);
    }

    pidRumo.definirGanhos(KP, KI, KD);
    pidRumo.definirLimiteSaida(-LIMITE_CORRECAO, LIMITE_CORRECAO);
    pidRumo.definirLimiteIntegral(LIMITE_INTEGRAL);

    Serial.println("[CENTRO2] Calibrando IMU — mantenha o robo PARADO...");
    imuCalibrarOffsetZ(300);

    conectarWiFi();

    Serial.println();
    logLinha("[CENTRO2] Pronto. Comece DESLOCADO num lado (parede dos 2 lados).");
    logLinha("[CENTRO2] <num>=cm | s=repete | kp <n> | vm <n> | db <n> | t=ToF | p=aborta");
    logLinha(String("[CENTRO2] KP_POS=") + String(KP_POS, 3) + " VIES_MAX=" + String(VIES_RUMO_MAX, 1)
             + " DEADBAND=" + String(DEADBAND_POS, 1) + " DIF_ALVO=" + String(DIF_ALVO, 0));
}

void loop() {
    if (server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            logLinha("=== Conectado a bancada de centralizacao (cascata) ===");
            logLinha("<num>=cm | s=repete | kp <n> | vm <n> | db <n> | t=ToF | p=aborta");
        } else {
            server.available().stop();
        }
    }

    static String bufCmd = "";
    char c;
    while (lerComando(c)) {
        if (c != '\n' && c != '\r') {
            if (bufCmd.length() < 16) bufCmd += c;
            continue;
        }
        bufCmd.trim();
        if (bufCmd.length() == 0) { continue; }

        String low = bufCmd; low.toLowerCase();

        if (low == "s") {
            logLinha(String("[CENTRO2] Rodando ") + String(DISTANCIA_ALVO_CM, 1) + " cm...");
            executarReta();
            despejarBuffer();
        } else if (low == "t") {
            monitorToF();
        } else if (low.startsWith("kp")) {
            KP_POS = bufCmd.substring(2).toFloat();
            logLinha(String("[CENTRO2] KP_POS ajustado para ") + String(KP_POS, 3));
        } else if (low.startsWith("vm")) {
            VIES_RUMO_MAX = bufCmd.substring(2).toFloat();
            logLinha(String("[CENTRO2] VIES_RUMO_MAX ajustado para ") + String(VIES_RUMO_MAX, 1) + " graus");
        } else if (low.startsWith("db")) {
            DEADBAND_POS = bufCmd.substring(2).toFloat();
            logLinha(String("[CENTRO2] DEADBAND_POS ajustado para ") + String(DEADBAND_POS, 1) + " (unid. difC)");
        } else {
            const float d = bufCmd.toFloat();
            if (d > 0.5f) {
                DISTANCIA_ALVO_CM = d;
                logLinha(String("[CENTRO2] Rodando ") + String(DISTANCIA_ALVO_CM, 1) + " cm...");
                executarReta();
                despejarBuffer();
            }
        }
        bufCmd = "";
    }

    delay(5);
}

void executarReta() {
    encodersZerar();
    imuZerarAnguloZ();
    pidRumo.resetar();
    nAmostras = 0;

    const uint32_t t0 = millis();
    uint32_t ultimo   = t0;
    bool abortou = false;

    while (true) {
        imuAtualizar();
        motoresAtualizar();

        const uint32_t agora = millis();
        if (agora - ultimo < INTERVALO_MS) { delay(1); continue; }
        const float dt = (agora - ultimo) / 1000.0f;
        ultimo = agora;

        char cmd;
        if (lerComando(cmd) && (cmd == 'p' || cmd == 'P')) { abortou = true; break; }

        const int32_t eEsq = encoderLerEsquerdo();
        const int32_t eDir = encoderLerDireito();
        const float distMedia = 0.5f * (encoderDistanciaEsquerdaCm() + encoderDistanciaDireitaCm());
        const float ang = imuLerAnguloZ();

        if (distMedia >= DISTANCIA_ALVO_CM) break;
        if (agora - t0 > TIMEOUT_MS) { logLinha("[CENTRO2] TIMEOUT."); break; }

        // --- Leitura dos ToF com sanidade ---
        const uint16_t esqCorr = tofLerDistancia(1);        // esquerda corrigida (~= cru)
        const uint16_t dirCru  = tofLerDistanciaBruta(2);   // direita CRUA (sinal da centralização)
        const uint16_t dirCorr = tofLerDistancia(2);        // direita corrigida (só p/ gatear parede)

        // Só centraliza com PAREDE dos dois lados (pela distância CORRIGIDA) e leitura válida.
        const bool paredeEsq = tofValido(esqCorr) && esqCorr < LIMIAR_PAREDE;
        const bool paredeDir = tofValido(dirCorr) && dirCorr < LIMIAR_PAREDE;
        const bool gate      = paredeEsq && paredeDir && tofValido(dirCru);

        // --- LAÇO EXTERNO (posição -> viés de rumo) ---
        float difC     = 0.0f;
        float erroPos  = 0.0f;
        float alvoRumo = 0.0f;   // corredor reto: baseline do rumo é 0
        if (gate) {
            difC    = (float)esqCorr - (float)dirCru;
            erroPos = difC - DIF_ALVO;             // <0 = muito à esquerda; >0 = à direita
            // Deadband CONTÍNUO: zera dentro da zona morta e, fora dela, começa do
            // zero (subtrai a borda) -> sem degrau no limite. Mata a micro-serpenteada
            // do fim sem atrasar a aproximação (longe do centro, quase não pesa).
            float erroEff = erroPos;
            if (fabsf(erroEff) <= DEADBAND_POS) {
                erroEff = 0.0f;
            } else {
                erroEff -= (erroEff > 0.0f) ? DEADBAND_POS : -DEADBAND_POS;
            }
            alvoRumo = KP_POS * erroEff;           // <0 -> nariz p/ direita (IMU diminui)
            if (alvoRumo >  VIES_RUMO_MAX) alvoRumo =  VIES_RUMO_MAX;
            if (alvoRumo < -VIES_RUMO_MAX) alvoRumo = -VIES_RUMO_MAX;
        }

        // --- LAÇO INTERNO (PID de rumo persegue o alvo enviesado) ---
        const float erroRumo = alvoRumo - ang;
        const float correcao = pidRumo.calcular(erroRumo, dt);

        // Perfil de velocidade: desacelera nos últimos DECEL_CM.
        int16_t velCruzeiro = PWM_BASE;
        const float restante = DISTANCIA_ALVO_CM - distMedia;
        if (restante < DECEL_CM) {
            velCruzeiro = VEL_MIN + (int16_t)((PWM_BASE - VEL_MIN) * (restante / DECEL_CM));
            if (velCruzeiro < VEL_MIN) velCruzeiro = VEL_MIN;
        }
        motoresSetCruzeiro(velCruzeiro);
        motoresSetCorrecao((int16_t)correcao);

        if (nAmostras < MAX_AMOSTRAS) {
            Amostra& a = buffer[nAmostras++];
            a.t_ms     = agora - t0;
            a.encEsq   = eEsq;  a.encDir = eDir;
            a.ang_z    = ang;   a.alvoRumo = alvoRumo;
            a.difC     = difC;  a.erroPos = erroPos;   a.correcao = correcao;
            a.esq_mm   = esqCorr;  a.dir_mm = dirCru;  a.gate = gate ? 1 : 0;
            a.pwmEsq   = motorLerVelocidadeAtual(MOTOR_ESQUERDO);
            a.pwmDir   = motorLerVelocidadeAtual(MOTOR_DIREITO);
        }
    }

    // Freio ativo.
    motoresFrear();
    const uint32_t tf = millis();
    while (millis() - tf < FREIO_MS) {
        imuAtualizar();
        delay(2);
    }
    motoresParar();
    motoresAtualizar();

    // Resumo: onde começou vs onde terminou o difC (centralizou?).
    if (nAmostras > 0) {
        const float difIni = buffer[0].difC;
        const float difFim = buffer[nAmostras - 1].difC;
        logLinha(String("[CENTRO2] difC inicio=") + String(difIni, 0)
                 + " fim=" + String(difFim, 0) + " (alvo " + String(DIF_ALVO, 0)
                 + ") | rumo final=" + String(imuLerAnguloZ(), 2) + " deg");
    }
    if (abortou) logLinha("[CENTRO2] ABORTADO pelo usuario.");
}

// Monitor de ToF ao vivo — reposicionar o robô e conferir leituras.
void monitorToF() {
    logLinha("[TOF] Monitor ao vivo (mm). Mova o robo; 'p' pra sair.");
    logLinha("[TOF] esq corr/cru | dir corr/cru | fre corr/cru | difC(esq_corr - dir_cru)");
    while (true) {
        char c;
        if (lerComando(c) && (c == 'p' || c == 'P')) break;
        const uint16_t ec = tofLerDistancia(1), er = tofLerDistanciaBruta(1);
        const uint16_t dc = tofLerDistancia(2), dr = tofLerDistanciaBruta(2);
        const uint16_t fc = tofLerDistancia(0), fr = tofLerDistanciaBruta(0);
        char linha[128];
        snprintf(linha, sizeof(linha),
                 "[TOF] esq %4u/%4u | dir %4u/%4u | fre %4u/%4u | difC %d",
                 ec, er, dc, dr, fc, fr, (int)ec - (int)dr);
        logLinha(String(linha));
        delay(200);
    }
    logLinha("[TOF] Monitor encerrado.");
}

void despejarBuffer() {
    logLinha("---- INICIO TELEMETRIA (CSV) ----");
    logLinha("t_ms,encEsq,encDir,ang_z,alvoRumo,difC,erroPos,correcao,esq_mm,dir_mm,gate,pwmEsq,pwmDir");
    char linha[176];
    for (int i = 0; i < nAmostras; i++) {
        const Amostra& a = buffer[i];
        snprintf(linha, sizeof(linha),
                 "%lu,%ld,%ld,%.2f,%.2f,%.1f,%.1f,%.2f,%u,%u,%u,%d,%d",
                 (unsigned long)a.t_ms, (long)a.encEsq, (long)a.encDir,
                 a.ang_z, a.alvoRumo, a.difC, a.erroPos, a.correcao,
                 a.esq_mm, a.dir_mm, a.gate, a.pwmEsq, a.pwmDir);
        logLinha(String(linha));
    }
    char resumo[64];
    snprintf(resumo, sizeof(resumo), "---- FIM (%d amostras) ----", nAmostras);
    logLinha(String(resumo));
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
    Serial.printf("[CENTRO2] Conectando em \"%s\" (DHCP) ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[CENTRO2] Wi-Fi OK. Conecte em  %s:%u\n",
                      WiFi.localIP().toString().c_str(), WIFI_PORTA);
        server.begin();
    } else {
        Serial.println("\n[CENTRO2] Wi-Fi FALHOU. Cheque SSID/senha e se a rede e 2,4GHz.");
    }
}
