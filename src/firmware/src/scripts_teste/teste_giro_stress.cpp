#include <Arduino.h>
#include <WiFi.h>
#include "atuadores/motores.h"
#include "sensores/encoders.h"
#include "sensores/imu.h"
#include "navegacao/pid.h"

// =============================================================================
// BANCADA DE GIRO — STRESS (Wi-Fi puro, sem depender do serial)
//
// Objetivo: ESTRESSAR os motores/alimentacao e capturar a falha de TORQUE que
// aparece nos logs (o software manda PWM alto, mas as rodas NAO giram:
// velGiro ~ 0 e encoders congelados). Feito pra rodar SO pelo Wi-Fi:
//   - IP ESTATICO fixo  -> voce SEMPRE conecta no mesmo endereco, sem serial.
//   - Comandos e telemetria 100% por TCP (porta 8080).
//
// Comandos (mande pela captura_log ou nc):
//   1:+90  2:-90  3:+180  4:-180   -> giro unico (baseline)
//   5 -> STRESS: 10 giros de 90 alternados, em sequencia, tudo logado.
//        Se a bateria/motor estiver fraco, o angulo final CAI a cada giro.
//   6 -> SONDA DE TORQUE: rampa o PWM de 0 ate o maximo girando no lugar e
//        registra a resposta. Breakaway alto / velGiro baixo = falta torque.
//   p -> aborta o movimento em andamento.
//
// Resultado: cada modo despeja um CSV (marcadores INICIO/FIM) capturavel pela
// captura_log; os resumos por turno saem ao vivo no terminal.
// =============================================================================

// --- Wi-Fi (DHCP): pega o IP do roteador; leia o IP no serial no boot. --------
const char*    WIFI_SSID      = "VIVOFIBRA-8681";    // <-- coloque o nome da rede/hotspot
const char*    WIFI_PASS      = "DSCBCkm8r3";   // <-- coloque a senha
const uint16_t WIFI_PORTA      = 8080;
const uint32_t WIFI_TIMEOUT_MS = 8000;
WiFiServer server(WIFI_PORTA);
WiFiClient client;

// --- Controle (mesmos parametros do teste_giro validado) ---------------------
const uint16_t INTERVALO_MS  = 10;
const uint32_t TIMEOUT_MS    = 4000;
const float    TOL_ANGULO    = 4.5f;
const float    TOL_VEL_GIRO  = 15.0f;
const uint8_t  CICLOS_ESTAVEL = 3;
const float    VEL_PARADO    = 4.0f;    // °/s abaixo disso = "parado"
const uint8_t  CICLOS_PARADO = 40;      // ~0,4 s parado -> encerra o giro

float KP = 5.0f, KI = 8.0f, KD = 0.3f;
const float LIMITE_SAIDA    = 150.0f;
const float LIMITE_INTEGRAL = 15.0f;
const float PWM_MIN_GIRO    = 90.0f;    // piso quebra-atrito no fim do giro

// --- Desaceleração angular (mata o overshoot: chega devagar no alvo) ---
const float GRAUS_DECEL  = 40.0f;   // começa a frear a rotação nos últimos N graus
const float VEL_GIRO_MIN = 95.0f;   // teto mínimo perto do alvo (>= PWM_MIN_GIRO)

// --- Stress / sonda ---
const int      STRESS_TURNOS = 10;      // nº de giros de 90 no comando '5'
const uint32_t PROBE_MS      = 2500;    // duracao da rampa de torque no '6'

PID pidGiro(KP, KI, KD);

struct Amostra {
    uint32_t t_ms;
    float    ang_z, erro, velGiro, correcao;
    int16_t  pwmEsq, pwmDir;
    int32_t  encEsq, encDir;
};
const int MAX_AMOSTRAS = 1000;          // ~13 s de amostras (~32 KB de RAM)
Amostra buffer[MAX_AMOSTRAS];
int      nAmostras = 0;
uint32_t t0Global  = 0;                 // referencia de tempo comum do comando

struct Resultado {
    float       alvo, final_, erro;
    const char* motivo;
    float       dtMax;
    uint16_t    perdidos;
};

// --- Prototipos ---
void      conectarWiFi();
bool      lerComando(char& c);
void      logLinha(const String& s);
Resultado girar(float alvoAbs);
void      sondaTorque();
void      frear(uint32_t maxMs);
void      despejarBuffer(const char* nome);

void setup() {
    Serial.begin(115200);
    delay(200);

    motoresInit();
    encodersInit();
    if (!imuInit()) {
        Serial.println("[STRESS] ERRO: IMU nao inicializou. Travado.");
        while (true) delay(1000);
    }

    pidGiro.definirGanhos(KP, KI, KD);
    pidGiro.definirLimiteSaida(-LIMITE_SAIDA, LIMITE_SAIDA);
    pidGiro.definirLimiteIntegral(LIMITE_INTEGRAL);

    Serial.println("[STRESS] Calibrando IMU — mantenha o robo PARADO...");
    imuCalibrarOffsetZ(300);

    conectarWiFi();
    logLinha("[STRESS] Pronto. 1/2/3/4=giro unico | 5=stress | 6=sonda torque | p=aborta");
}

void loop() {
    if (server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            logLinha("=== Conectado a bancada de STRESS ===");
            logLinha("1:+90 2:-90 3:+180 4:-180 | 5:stress(10x90) | 6:sonda torque");
        } else {
            server.available().stop();
        }
    }

    char c;
    if (!lerComando(c)) { delay(5); return; }

    // Drena o resto da entrada (newline, bytes duplicados) -> 1 tecla = 1 acao.
    while (Serial.available() > 0) Serial.read();
    while (client && client.connected() && client.available() > 0) client.read();

    if (c == '1' || c == '2' || c == '3' || c == '4') {
        const float delta = (c == '1') ? 90.0f : (c == '2') ? -90.0f
                          : (c == '3') ? 180.0f : -180.0f;
        imuZerarAnguloZ(); encodersZerar(); nAmostras = 0; t0Global = millis();
        Resultado r = girar(delta);
        logLinha(String("[GIRO] alvo=") + String(r.alvo, 0) + " final=" + String(r.final_, 2)
                 + " erro=" + String(r.erro, 2) + " fim=" + r.motivo
                 + " | dtMax=" + String(r.dtMax * 1000.0f, 0) + "ms perdidosIMU=" + String(r.perdidos));
        despejarBuffer("giro_unico");
    }
    else if (c == '5') {
        imuZerarAnguloZ(); encodersZerar(); nAmostras = 0; t0Global = millis();
        logLinha(String("[STRESS] Sequencia de ") + STRESS_TURNOS + " giros de 90 alternados...");
        float alvo = 0.0f;
        for (int i = 0; i < STRESS_TURNOS; i++) {
            alvo += (i % 2 == 0) ? +90.0f : -90.0f;   // vai a 90 e volta a 0, alternando
            Resultado r = girar(alvo);
            logLinha(String("[STRESS] turno ") + String(i + 1) + "/" + STRESS_TURNOS
                     + " alvo=" + String(r.alvo, 0) + " final=" + String(r.final_, 2)
                     + " erro=" + String(r.erro, 2) + " fim=" + r.motivo);
        }
        logLinha("[STRESS] Fim da sequencia.");
        despejarBuffer("giro_stress");
    }
    else if (c == '6') {
        imuZerarAnguloZ(); encodersZerar(); nAmostras = 0; t0Global = millis();
        logLinha("[STRESS] Sonda de torque: rampa de PWM 0 -> max girando no lugar...");
        sondaTorque();
        despejarBuffer("sonda_torque");
    }
}

// -----------------------------------------------------------------------------
// Gira ate o ANGULO ABSOLUTO alvoAbs (nao zera a IMU: a sequencia acumula).
// Appenda amostras no buffer global. Nao passa de MAX_AMOSTRAS.
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

        // Conclusao normal.
        if (fabsf(erro) <= TOL_ANGULO && fabsf(velGiro) <= TOL_VEL_GIRO) {
            if (++estavel >= CICLOS_ESTAVEL) { res.motivo = "ok"; break; }
        } else {
            estavel = 0;
        }

        // Fallback: travou (atrito/torque venceu) fora da tolerancia.
        if (fabsf(velGiro) > VEL_PARADO) arrancou = true;
        if (arrancou && fabsf(velGiro) < VEL_PARADO && fabsf(erro) > TOL_ANGULO) {
            if (++parado >= CICLOS_PARADO) { res.motivo = "travou"; break; }
        } else {
            parado = 0;
        }

        if (agora - t0 > TIMEOUT_MS) { res.motivo = "timeout"; break; }

        float saida = pidGiro.calcular(erro, dt);
        // Piso quebra-atrito: garante o mínimo pra completar o fim do giro.
        if (fabsf(erro) > TOL_ANGULO && fabsf(saida) < PWM_MIN_GIRO)
            saida = (erro > 0.0f) ? PWM_MIN_GIRO : -PWM_MIN_GIRO;
        // Teto que DESACELERA perto do alvo: reduz a velocidade nos últimos
        // GRAUS_DECEL pra chegar devagar e não passar (overshoot).
        float teto = LIMITE_SAIDA;
        if (fabsf(erro) < GRAUS_DECEL)
            teto = VEL_GIRO_MIN + (LIMITE_SAIDA - VEL_GIRO_MIN) * (fabsf(erro) / GRAUS_DECEL);
        if (saida >  teto) saida =  teto;
        if (saida < -teto) saida = -teto;
        motoresSetCruzeiro(0);
        motoresSetCorrecao((int16_t)saida);

        if (nAmostras < MAX_AMOSTRAS) {
            Amostra& a = buffer[nAmostras++];
            a.t_ms     = agora - t0Global;
            a.ang_z    = ang;   a.erro = erro;   a.velGiro = velGiro;   a.correcao = saida;
            a.pwmEsq   = motorLerVelocidadeAtual(MOTOR_ESQUERDO);
            a.pwmDir   = motorLerVelocidadeAtual(MOTOR_DIREITO);
            a.encEsq   = encoderLerEsquerdo();  a.encDir = encoderLerDireito();
        }
    }

    frear(1000);
    res.final_ = imuLerAnguloZ();
    res.erro   = alvoAbs - res.final_;
    return res;
}

// -----------------------------------------------------------------------------
// Sonda de torque: rampa o PWM de rotacao de 0 ate LIMITE_SAIDA e loga a
// resposta. No CSV, veja em que 'correcao' (PWM) o 'velGiro' comeca a subir:
// esse e o breakaway. PWM alto sem velGiro = motor/bateria/driver sem torque.
// -----------------------------------------------------------------------------
void sondaTorque() {
    pidGiro.resetar();
    const uint32_t t0 = millis();
    uint32_t ultimo   = t0;

    while (true) {
        imuAtualizar();
        motoresAtualizar();

        const uint32_t agora = millis();
        if (agora - ultimo < INTERVALO_MS) { delay(1); continue; }
        ultimo = agora;

        const uint32_t elapsed = agora - t0;
        if (elapsed > PROBE_MS) break;

        char cmd;
        if (lerComando(cmd) && (cmd == 'p' || cmd == 'P')) break;

        const float frac = (float)elapsed / (float)PROBE_MS;
        const float pwm  = frac * LIMITE_SAIDA;      // rampa 0..LIMITE_SAIDA
        motoresSetCruzeiro(0);
        motoresSetCorrecao((int16_t)pwm);

        const float ang     = imuLerAnguloZ();
        const float velGiro = imuLerGiroZ();
        if (nAmostras < MAX_AMOSTRAS) {
            Amostra& a = buffer[nAmostras++];
            a.t_ms     = agora - t0Global;
            a.ang_z    = ang;   a.erro = 0.0f;   a.velGiro = velGiro;   a.correcao = pwm;
            a.pwmEsq   = motorLerVelocidadeAtual(MOTOR_ESQUERDO);
            a.pwmDir   = motorLerVelocidadeAtual(MOTOR_DIREITO);
            a.encEsq   = encoderLerEsquerdo();  a.encDir = encoderLerDireito();
        }
    }
    frear(1000);
}

// -----------------------------------------------------------------------------
void frear(uint32_t maxMs) {
    motoresParar();
    const uint32_t tf = millis();
    while ((motorLerVelocidadeAtual(MOTOR_ESQUERDO) != 0 ||
            motorLerVelocidadeAtual(MOTOR_DIREITO)  != 0) &&
           millis() - tf < maxMs) {
        motoresAtualizar();
        delay(2);
    }
}

void despejarBuffer(const char* nome) {
    logLinha(String("---- INICIO TELEMETRIA (CSV) [") + nome + "] ----");
    logLinha("t_ms,ang_z,erro,velGiro,correcao,pwmEsq,pwmDir,encEsq,encDir");
    char linha[128];
    for (int i = 0; i < nAmostras; i++) {
        const Amostra& a = buffer[i];
        snprintf(linha, sizeof(linha), "%lu,%.2f,%.2f,%.2f,%.2f,%d,%d,%ld,%ld",
                 (unsigned long)a.t_ms, a.ang_z, a.erro, a.velGiro, a.correcao,
                 a.pwmEsq, a.pwmDir, (long)a.encEsq, (long)a.encDir);
        logLinha(String(linha));
    }
    logLinha(String("---- FIM (") + String(nAmostras) + " amostras) ----");
}

// -----------------------------------------------------------------------------
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
    Serial.printf("[STRESS] Conectando em \"%s\" (DHCP) ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);   // sem modem-sleep: telemetria ao vivo fluida
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[STRESS] Wi-Fi OK. Conecte em  %s:%u\n",
                      WiFi.localIP().toString().c_str(), WIFI_PORTA);
        server.begin();
    } else {
        Serial.println("\n[STRESS] Wi-Fi FALHOU. Cheque SSID/senha e se o hotspot e 2,4GHz.");
    }
}
