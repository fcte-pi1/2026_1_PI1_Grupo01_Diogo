#include <Arduino.h>
#include <WiFi.h>
#include "atuadores/motores.h"
#include "sensores/encoders.h"
#include "sensores/imu.h"
#include "navegacao/pid.h"

// =============================================================================
// BANCADA DE RETA (rumo por IMU)
//
// Objetivo: validar/tunar o PID de manutenção de rumo de forma ISOLADA.
//   - Controle: erro = 0 - anguloZ(IMU)  -> correcao diferencial.
//   - Atuador : motoresSetCruzeiro (base rampeada) + motoresSetCorrecao (imediata).
//   - Telemetria: gravada num BUFFER em RAM durante o movimento (zero I/O no laço),
//     despejada em CSV só no fim (Serial e, se conectado, Wi-Fi). Isso mantém o
//     dt do controle limpo — nada de println bloqueante dentro do loop.
//
// Como usar: robô no chão com ~70 cm livres à frente. Ligue, deixe PARADO durante
// a calibração da IMU, e envie 's' (pelo Serial Monitor ou por TCP na porta 8080).
// Envie 'p' durante o movimento para abortar.
// =============================================================================

// --- Wi-Fi (opcional; se falhar, cai pra Serial e segue funcionando) ---
const bool     WIFI_ATIVA     = true;
const char*    WIFI_SSID      = "VIVOFIBRA-8681";    // <-- coloque o nome da rede/hotspot
const char*    WIFI_PASS      = "DSCBCkm8r3";   // <-- coloque a senha
const uint16_t WIFI_PORTA     = 8080;
const uint32_t WIFI_TIMEOUT_MS = 8000;
WiFiServer server(WIFI_PORTA);
WiFiClient client;

// --- Parâmetros do teste ---
float          DISTANCIA_ALVO_CM = 40.0f;   // mutável: mande um número pelo terminal p/ mudar o alvo
const int16_t  PWM_BASE          = 120;    // velocidade de cruzeiro
const uint16_t INTERVALO_MS      = 10;     // período do controle
const uint32_t TIMEOUT_MS        = 8000;   // segurança: aborta se demorar demais

// --- Desaceleração + freio ativo (mata o coast/inércia no fim) ---
const int16_t  VEL_MIN   = 50;     // piso do cruzeiro ao desacelerar
const float    DECEL_CM  = 8.0f;   // desacelera nos últimos N cm antes do alvo
const uint32_t FREIO_MS  = 250;    // duração do short-brake no fim

// --- Detecção de stall (rodas travadas: bateu na parede / enroscou) ---
// Se os encoders nao avancam STALL_PULSOS_MIN por STALL_MS enquanto o robo ja
// deveria estar andando, encerra na hora em vez de ralar o motor na parede.
const int32_t  STALL_PULSOS_MIN  = 3;      // progresso minimo p/ contar como "andando"
const uint32_t STALL_MS          = 150;    // travado por esse tempo => aborta
const int32_t  STALL_ARM_PULSOS  = 20;     // so arma depois que o robo saiu do lugar

// --- Ganhos do PID de rumo ---
// ATENÇÃO: agora a correção é IMEDIATA (sem rampa), então o Kp "pega" muito mais
// forte que no código antigo. Começamos BAIXO de propósito. Suba aos poucos.
float KP = 6.0f;
float KI = 1.5f;
float KD = 0.3f;
const float LIMITE_CORRECAO = 70.0f;   // satura o diferencial (PWM)
const float LIMITE_INTEGRAL = 40.0f;

PID pidRumo(KP, KI, KD);

// --- Buffer de telemetria (em RAM) ---
struct Amostra {
    uint32_t t_ms;
    int32_t  encEsq;      // pulsos crus (melhor p/ enxergar glitch de encoder)
    int32_t  encDir;
    float    distEsq_cm;
    float    distDir_cm;
    float    ang_z;
    float    erro;
    float    correcao;
    int16_t  pwmEsq;      // saída realmente aplicada (inclui rampa da base)
    int16_t  pwmDir;
};
const int MAX_AMOSTRAS = 600;   // ~6 s a 10 ms; ~21 KB de RAM
Amostra buffer[MAX_AMOSTRAS];
int nAmostras = 0;

// --- Protótipos ---
void conectarWiFi();
void executarReta();
void despejarBuffer();
void logLinha(const String& s);
bool lerComando(char& c);

void setup() {
    Serial.begin(115200);
    delay(200);

    motoresInit();
    encodersInit();

    if (!imuInit()) {
        Serial.println("[BANCADA] ERRO: IMU nao inicializou. Travado.");
        while (true) delay(1000);
    }

    pidRumo.definirGanhos(KP, KI, KD);
    pidRumo.definirLimiteSaida(-LIMITE_CORRECAO, LIMITE_CORRECAO);
    pidRumo.definirLimiteIntegral(LIMITE_INTEGRAL);

    Serial.println("[BANCADA] Calibrando IMU — mantenha o robo PARADO...");
    imuCalibrarOffsetZ(300);

    if (WIFI_ATIVA) conectarWiFi();

    Serial.println();
    Serial.println("[BANCADA] Pronto. Envie 's' (Serial ou Wi-Fi) para rodar a reta.");
}

void loop() {
    // Aceita/atualiza cliente Wi-Fi.
    if (WIFI_ATIVA && server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            logLinha("=== Conectado a bancada de reta ===");
            logLinha("Envie 's' para rodar. 'p' aborta durante o movimento.");
        } else {
            server.available().stop();
        }
    }

    // Acumula uma linha: NÚMERO = distância alvo (cm); 's' = repetir a atual.
    static String bufCmd = "";
    char c;
    while (lerComando(c)) {
        if (c != '\n' && c != '\r') {
            if (bufCmd.length() < 12) bufCmd += c;
            continue;
        }
        bufCmd.trim();
        bool rodar = false;
        if (bufCmd == "s" || bufCmd == "S") {
            rodar = true;                       // repete com a distância atual
        } else if (bufCmd.length() > 0) {
            const float d = bufCmd.toFloat();
            if (d > 0.5f) { DISTANCIA_ALVO_CM = d; rodar = true; }
        }
        bufCmd = "";
        if (rodar) {
            logLinha(String("[BANCADA] Rodando ") + String(DISTANCIA_ALVO_CM, 1) + " cm...");
            executarReta();
            despejarBuffer();
            logLinha("[BANCADA] Fim. Mande um numero (cm) ou 's' pra repetir.");
        }
    }

    delay(5);
}

// Lê um comando do Serial OU do cliente Wi-Fi (o que chegar primeiro).
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
    Serial.printf("[BANCADA] Conectando ao Wi-Fi \"%s\" ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[BANCADA] Wi-Fi OK. IP: %s  (porta %u)\n",
                      WiFi.localIP().toString().c_str(), WIFI_PORTA);
        server.begin();
    } else {
        Serial.println("\n[BANCADA] Wi-Fi falhou no tempo limite. Seguindo SO por Serial.");
    }
}

void executarReta() {
    // Zera referências.
    encodersZerar();
    imuZerarAnguloZ();
    pidRumo.resetar();
    nAmostras = 0;

    const float anguloAlvo = 0.0f;   // manter o rumo inicial
    const uint32_t t0 = millis();
    uint32_t ultimo   = t0;
    bool abortou = false;

    // Estado da deteccao de stall.
    int32_t  encAnterior  = 0;       // soma esq+dir na ultima verificacao
    uint32_t ultimoAvanco = t0;      // instante do ultimo progresso dos encoders

    logLinha("[BANCADA] Rodando... (log so no fim)");

    while (true) {
        // Mantém IMU integrando e motores aplicando saída a cada passada.
        imuAtualizar();
        motoresAtualizar();

        const uint32_t agora = millis();
        if (agora - ultimo < INTERVALO_MS) { delay(1); continue; }
        const float dt = (agora - ultimo) / 1000.0f;
        ultimo = agora;

        // Aborto manual.
        char cmd;
        if (lerComando(cmd) && (cmd == 'p' || cmd == 'P')) { abortou = true; break; }

        // Medidas.
        const int32_t eEsq = encoderLerEsquerdo();
        const int32_t eDir = encoderLerDireito();
        const float distEsq = encoderDistanciaEsquerdaCm();
        const float distDir = encoderDistanciaDireitaCm();
        const float distMedia = 0.5f * (distEsq + distDir);
        const float ang = imuLerAnguloZ();

        // Parada por distância.
        if (distMedia >= DISTANCIA_ALVO_CM) break;
        // Segurança por tempo.
        if (agora - t0 > TIMEOUT_MS) { logLinha("[BANCADA] TIMEOUT."); break; }

        // Segurança por stall: rodas paradas com o robô já em movimento => bateu/enroscou.
        const int32_t encSoma  = eEsq + eDir;
        const int32_t encDelta = encSoma > encAnterior ? encSoma - encAnterior
                                                        : encAnterior - encSoma;
        if (encDelta >= STALL_PULSOS_MIN) {
            encAnterior  = encSoma;   // houve progresso: rearma o cronômetro
            ultimoAvanco = agora;
        } else if (encSoma > STALL_ARM_PULSOS && agora - ultimoAvanco > STALL_MS) {
            logLinha("[BANCADA] STALL: encoders parados, encerrando.");
            break;
        }

        // --- Controle de rumo ---
        const float erro = anguloAlvo - ang;
        const float correcao = pidRumo.calcular(erro, dt);

        // Perfil de velocidade: cruzeiro cheio, desacelerando nos últimos
        // DECEL_CM pra chegar devagar no alvo (coast mínimo, rumo sob controle).
        int16_t velCruzeiro = PWM_BASE;
        const float restante = DISTANCIA_ALVO_CM - distMedia;
        if (restante < DECEL_CM) {
            velCruzeiro = VEL_MIN + (int16_t)((PWM_BASE - VEL_MIN) * (restante / DECEL_CM));
            if (velCruzeiro < VEL_MIN) velCruzeiro = VEL_MIN;
        }
        motoresSetCruzeiro(velCruzeiro);
        motoresSetCorrecao((int16_t)correcao);

        // Registra amostra (sem nenhum I/O).
        if (nAmostras < MAX_AMOSTRAS) {
            Amostra& a = buffer[nAmostras++];
            a.t_ms       = agora - t0;
            a.encEsq     = eEsq;
            a.encDir     = eDir;
            a.distEsq_cm = distEsq;
            a.distDir_cm = distDir;
            a.ang_z      = ang;
            a.erro       = erro;
            a.correcao   = correcao;
            a.pwmEsq     = motorLerVelocidadeAtual(MOTOR_ESQUERDO);
            a.pwmDir     = motorLerVelocidadeAtual(MOTOR_DIREITO);
        }
    }

    // Frenagem ATIVA: short-brake pra matar o rolamento livre. Como já chegamos
    // devagar (perfil de desaceleração), o freio é suave e trava o resíduo.
    motoresFrear();
    const uint32_t tf = millis();
    while (millis() - tf < FREIO_MS) {
        imuAtualizar();
        delay(2);
    }
    motoresParar();
    motoresAtualizar();

    if (abortou) logLinha("[BANCADA] ABORTADO pelo usuario.");
}

void despejarBuffer() {
    logLinha("---- INICIO TELEMETRIA (CSV) ----");
    logLinha("t_ms,encEsq,encDir,distEsq_cm,distDir_cm,ang_z,erro,correcao,pwmEsq,pwmDir");

    char linha[128];
    for (int i = 0; i < nAmostras; i++) {
        const Amostra& a = buffer[i];
        snprintf(linha, sizeof(linha),
                 "%lu,%ld,%ld,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d",
                 (unsigned long)a.t_ms, (long)a.encEsq, (long)a.encDir,
                 a.distEsq_cm, a.distDir_cm, a.ang_z, a.erro, a.correcao,
                 a.pwmEsq, a.pwmDir);
        logLinha(String(linha));
    }

    char resumo[64];
    snprintf(resumo, sizeof(resumo), "---- FIM (%d amostras) ----", nAmostras);
    logLinha(String(resumo));
}
