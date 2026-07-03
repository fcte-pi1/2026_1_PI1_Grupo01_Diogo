#include <Arduino.h>
#include <WiFi.h>
#include "atuadores/motores.h"
#include "sensores/encoders.h"
#include "sensores/imu.h"
#include "navegacao/pid.h"

// =============================================================================
// BANCADA DE GIRO (rotação por IMU) — v2
//
// Mudanças desta versão, a partir dos logs:
//   - Velocidade de giro MENOR (LIMITE_SAIDA 130 -> 100): menos embalo, menos
//     overshoot no 180, menos derrapagem no arranque.
//   - Ki > 0 aproveitando o novo anti-windup condicional do PID: ganha
//     autoridade só no fim do giro pra vencer o atrito e não parar 2° curto.
//   - Tolerância um pouco maior + fallback de "parado": nunca mais fica os 5 s
//     do timeout sentado a 2° do alvo.
//
// Envie 's' para girar +ALVO_GRAUS. 'p' aborta.
// =============================================================================

const bool     WIFI_ATIVA      = true;
const char*    WIFI_SSID      = "Fernando";
const char*    WIFI_PASS      = "r53tgfd53t5e";
const uint16_t WIFI_PORTA      = 8080;
const uint32_t WIFI_TIMEOUT_MS = 8000;
WiFiServer server(WIFI_PORTA);
WiFiClient client;

// Alvo escolhido em tempo de execucao pelos comandos 1/2/3/4 (ver loop()).
const uint16_t INTERVALO_MS = 10;
const uint32_t TIMEOUT_MS   = 4000;

// Conclusao.
const float   TOL_ANGULO     = 4.5f;   // ° do alvo. Overshoot sistemático de ~3° no atrito do labirinto; a reta seguinte corrige o resíduo
const float   TOL_VEL_GIRO   = 15.0f;  // °/s
const uint8_t CICLOS_ESTAVEL = 3;
// Fallback: se travar (parado) perto do alvo, desiste de esperar.
const float   VEL_PARADO     = 4.0f;   // °/s abaixo disso = "parado"
const uint8_t CICLOS_PARADO  = 40;     // ~0,4 s parado -> encerra

// --- Ganhos do PID de curva (AJUSTE AQUI) ---
float KP = 5.0f;
float KI = 8.0f;
float KD = 0.3f;
const float LIMITE_SAIDA    = 150.0f;  // <- teto do giro. 100 travava no atrito do labirinto (girou so 18 graus); subido p/ vencer o atrito
const float LIMITE_INTEGRAL = 15.0f;

// Piso de PWM (quebra-atrito): no fim do giro a saida do PID cai abaixo do que o
// motor precisa pra reiniciar do repouso e trava ~6 graus antes do alvo. Enquanto
// |erro| > TOL_ANGULO, garante pelo menos PWM_MIN_GIRO no sentido do erro.
const float PWM_MIN_GIRO    = 90.0f;

PID pidGiro(KP, KI, KD);

struct Amostra {
    uint32_t t_ms;
    float    ang_z;
    float    erro;
    float    velGiro;
    float    correcao;
    int16_t  pwmEsq;
    int16_t  pwmDir;
    int32_t  encEsq;
    int32_t  encDir;
};
const int MAX_AMOSTRAS = 500;
Amostra buffer[MAX_AMOSTRAS];
int nAmostras = 0;

void conectarWiFi();
void executarGiro(float alvo);
void despejarBuffer();
void logLinha(const String& s);
bool lerComando(char& c);

void setup() {
    Serial.begin(115200);
    delay(200);

    motoresInit();
    encodersInit();

    if (!imuInit()) {
        Serial.println("[GIRO] ERRO: IMU nao inicializou. Travado.");
        while (true) delay(1000);
    }

    pidGiro.definirGanhos(KP, KI, KD);
    pidGiro.definirLimiteSaida(-LIMITE_SAIDA, LIMITE_SAIDA);
    pidGiro.definirLimiteIntegral(LIMITE_INTEGRAL);

    Serial.println("[GIRO] Calibrando IMU — mantenha o robo PARADO...");
    imuCalibrarOffsetZ(300);

    if (WIFI_ATIVA) conectarWiFi();

    Serial.println();
    Serial.println("[GIRO] Pronto.  1:+90  2:-90  3:+180  4:-180   ('p' aborta durante o giro)");
}

void loop() {
    if (WIFI_ATIVA && server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            logLinha("=== Conectado a bancada de giro ===");
            logLinha("1:+90  2:-90  3:+180  4:-180   ('p' aborta durante o giro)");
        } else {
            server.available().stop();
        }
    }

    char c;
    if (lerComando(c)) {
        float alvo = 0.0f;
        switch (c) {
            case '1': alvo =   90.0f; break;
            case '2': alvo =  -90.0f; break;
            case '3': alvo =  180.0f; break;
            case '4': alvo = -180.0f; break;
            default:  alvo =    0.0f; break;   // qualquer outra tecla: ignora
        }
        if (alvo != 0.0f) {
            logLinha(String("[GIRO] Comando: ") + String(alvo, 0) + " graus");
            executarGiro(alvo);
            despejarBuffer();
            logLinha("[GIRO] Fim.  1:+90  2:-90  3:+180  4:-180 para repetir.");
        }
    }
    delay(5);
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
    Serial.printf("[GIRO] Conectando ao Wi-Fi \"%s\" ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[GIRO] Wi-Fi OK. IP: %s  (porta %u)\n",
                      WiFi.localIP().toString().c_str(), WIFI_PORTA);
        server.begin();
    } else {
        Serial.println("\n[GIRO] Wi-Fi falhou. Seguindo SO por Serial.");
    }
}

void executarGiro(float alvo) {
    // Drena entrada pendente (newline do comando, bytes atrasados/duplicados da
    // rede) para que 1 tecla = 1 giro deliberado.
    while (Serial.available() > 0) Serial.read();
    while (client && client.connected() && client.available() > 0) client.read();

    encodersZerar();
    imuZerarAnguloZ();
    pidGiro.resetar();
    nAmostras = 0;

    const uint32_t t0 = millis();
    uint32_t ultimo   = t0;
    uint8_t estavel   = 0;
    uint16_t parado   = 0;
    bool arrancou = false;   // vira true quando o robô comeca a girar de fato
    bool abortou = false;
    const char* motivo = "alvo";

    logLinha("[GIRO] Girando... (log so no fim)");

    while (true) {
        imuAtualizar();
        motoresAtualizar();

        const uint32_t agora = millis();
        if (agora - ultimo < INTERVALO_MS) { delay(1); continue; }
        const float dt = (agora - ultimo) / 1000.0f;
        ultimo = agora;

        char cmd;
        if (lerComando(cmd) && (cmd == 'p' || cmd == 'P')) { abortou = true; motivo = "abortado"; break; }

        const float ang     = imuLerAnguloZ();
        const float velGiro = imuLerGiroZ();
        const float erro    = alvo - ang;

        // Conclusao normal: dentro da tolerancia e girando devagar.
        if (fabsf(erro) <= TOL_ANGULO && fabsf(velGiro) <= TOL_VEL_GIRO) {
            if (++estavel >= CICLOS_ESTAVEL) { motivo = "ok"; break; }
        } else {
            estavel = 0;
        }

        // Fallback: parado (atrito venceu) mas ainda fora da tolerancia.
        // Só arma DEPOIS que o robô arrancou, senão a rampa inicial (robô ainda
        // parado de propósito) dispara um "travou" falso e o giro nem acontece.
        if (fabsf(velGiro) > VEL_PARADO) arrancou = true;
        if (arrancou && fabsf(velGiro) < VEL_PARADO && fabsf(erro) > TOL_ANGULO) {
            if (++parado >= CICLOS_PARADO) { motivo = "travou"; break; }
        } else {
            parado = 0;
        }

        if (agora - t0 > TIMEOUT_MS) { motivo = "timeout"; break; }

        float saida = pidGiro.calcular(erro, dt);
        // Piso de PWM: vence o atrito estático/zona morta no fim do giro.
        if (fabsf(erro) > TOL_ANGULO && fabsf(saida) < PWM_MIN_GIRO) {
            saida = (erro > 0.0f) ? PWM_MIN_GIRO : -PWM_MIN_GIRO;
        }
        motoresSetCruzeiro(0);
        motoresSetCorrecao((int16_t)saida);

        if (nAmostras < MAX_AMOSTRAS) {
            Amostra& a = buffer[nAmostras++];
            a.t_ms     = agora - t0;
            a.ang_z    = ang;
            a.erro     = erro;
            a.velGiro  = velGiro;
            a.correcao = saida;
            a.pwmEsq   = motorLerVelocidadeAtual(MOTOR_ESQUERDO);
            a.pwmDir   = motorLerVelocidadeAtual(MOTOR_DIREITO);
            a.encEsq   = encoderLerEsquerdo();
            a.encDir   = encoderLerDireito();
        }
    }

    motoresParar();
    const uint32_t tf = millis();
    while ((motorLerVelocidadeAtual(MOTOR_ESQUERDO) != 0 ||
            motorLerVelocidadeAtual(MOTOR_DIREITO)  != 0) &&
           millis() - tf < 1200) {
        motoresAtualizar();
        delay(2);
    }

    const float angFinal = imuLerAnguloZ();
    char resumo[112];
    snprintf(resumo, sizeof(resumo),
             "[GIRO] Alvo %+.1f | Final %+.2f | Erro %+.2f | %lu ms | fim: %s",
             alvo, angFinal, alvo - angFinal,
             (unsigned long)(millis() - t0), motivo);
    logLinha(String(resumo));
}

void despejarBuffer() {
    logLinha("---- INICIO TELEMETRIA (CSV) ----");
    logLinha("t_ms,ang_z,erro,velGiro,correcao,pwmEsq,pwmDir,encEsq,encDir");
    char linha[128];
    for (int i = 0; i < nAmostras; i++) {
        const Amostra& a = buffer[i];
        snprintf(linha, sizeof(linha),
                 "%lu,%.2f,%.2f,%.2f,%.2f,%d,%d,%ld,%ld",
                 (unsigned long)a.t_ms, a.ang_z, a.erro, a.velGiro, a.correcao,
                 a.pwmEsq, a.pwmDir, (long)a.encEsq, (long)a.encDir);
        logLinha(String(linha));
    }
    char resumo[64];
    snprintf(resumo, sizeof(resumo), "---- FIM (%d amostras) ----", nAmostras);
    logLinha(String(resumo));
}