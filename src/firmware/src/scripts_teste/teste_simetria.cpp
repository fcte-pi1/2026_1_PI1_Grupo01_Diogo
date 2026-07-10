#include <Arduino.h>
#include <WiFi.h>
#include "atuadores/motores.h"
#include "sensores/encoders.h"

// =============================================================================
// BANCADA DE SIMETRIA DE TRAÇÃO
//
// Manda o MESMO PWM pros dois motores e mede, pelos ENCODERS, se as duas rodas
// andam igual. Se não andam, quantifica a assimetria (razão dir/esq) — é o
// "tamanho do prejuízo" do puxão pra um lado.
//
// SERVE EM DOIS CENÁRIOS (rode nos dois!):
//   - SUSPENSO (rodas no ar): isola MOTOR / mecânica passiva (rolamento, roçar).
//   - NO CHÃO: inclui CARGA/TRAÇÃO (roda torta, peso, atrito) — pode aparecer só aqui.
//   Se der igual no ar mas diferente no chão -> o problema é de CARGA/estrutura.
//
// CONSTÂNCIA (pré-requisito do feedforward): o comando 'r' varre 80/100/120 e
// mostra a razão em cada um. Razão ~igual nos três = assimetria CONSTANTE ->
// feedforward resolve. Razão variando = NÃO constante -> precisa PID de encoder.
//
// Comandos (nc <IP> 8080 ou captura_log):
//   <num> -> roda os DOIS motores pra frente nesse PWM por ~2,5 s e loga/reporta.
//   r     -> varredura 80,100,120 (checa se a assimetria é constante).
//   p     -> aborta.
// =============================================================================

// --- Wi-Fi (DHCP): leia o IP no serial no boot ---
const char*    WIFI_SSID       = "iPhone";
const char*    WIFI_PASS       = "06543210";
const uint16_t WIFI_PORTA      = 8080;
const uint32_t WIFI_TIMEOUT_MS = 8000;
WiFiServer server(WIFI_PORTA);
WiFiClient client;

// --- Parâmetros do teste ---
const uint32_t DURACAO_MS   = 2500;   // duração de cada disparo
const uint16_t INTERVALO_MS = 20;     // período de amostragem (~50 Hz)
const uint32_t JANELA_INI_MS = 1000;  // razão medida na janela [1s, fim] (pós-rampa)

struct Amostra {
    uint32_t t_ms;
    int32_t  encEsq, encDir;
    int16_t  pwmEsq, pwmDir;
};
const int MAX_AMOSTRAS = 200;
Amostra buffer[MAX_AMOSTRAS];
int nAmostras = 0;

// --- Protótipos ---
void conectarWiFi();
bool lerComando(char& c);
void logLinha(const String& s);
void rodarPwm(int pwm, bool comCSV);

void setup() {
    Serial.begin(115200);
    delay(200);
    motoresInit();
    encodersInit();
    conectarWiFi();
    logLinha("[SIM] Pronto. <num>=roda os 2 motores nesse PWM | r=varre 80/100/120 | p=aborta");
}

void loop() {
    if (server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            logLinha("=== Conectado a bancada de SIMETRIA ===");
            logLinha("<num>=PWM nos 2 motores | r=varredura 80/100/120 | p=aborta");
        } else {
            server.available().stop();
        }
    }

    static String bufCmd = "";
    char c;
    while (lerComando(c)) {
        if (c != '\n' && c != '\r') {
            if (bufCmd.length() < 12) bufCmd += c;
            continue;
        }
        bufCmd.trim();
        if (bufCmd.length() == 0) { continue; }

        if (bufCmd == "r" || bufCmd == "R") {
            logLinha("[SIM] Varredura de constancia (80, 100, 120)...");
            rodarPwm(80,  false);  delay(600);
            rodarPwm(100, false);  delay(600);
            rodarPwm(120, false);
            logLinha("[SIM] Fim da varredura. Razoes ~iguais = CONSTANTE (feedforward serve).");
        } else {
            const int pwm = bufCmd.toInt();
            if (pwm >= 40 && pwm <= 255) {
                rodarPwm(pwm, true);
            } else {
                logLinha("[SIM] PWM invalido (use 40..255).");
            }
        }
        bufCmd = "";
    }
    delay(5);
}

// Roda os dois motores pra FRENTE no mesmo PWM e mede os encoders.
void rodarPwm(int pwm, bool comCSV) {
    encodersZerar();
    nAmostras = 0;

    const uint32_t t0 = millis();
    uint32_t ultimo = t0;
    int32_t  eEsqJanela = 0, eDirJanela = 0;
    bool     janelaMarcada = false;
    bool     abortou = false;

    motoresSetCruzeiro(pwm);     // base rampeada, igual pros dois
    motoresSetCorrecao(0);       // sem diferencial: comando IDENTICO

    while (millis() - t0 < DURACAO_MS) {
        motoresAtualizar();

        char cmd;
        if (lerComando(cmd) && (cmd == 'p' || cmd == 'P')) { abortou = true; break; }

        const uint32_t agora = millis();
        if (agora - ultimo < INTERVALO_MS) { delay(1); continue; }
        ultimo = agora;

        motoresSetCruzeiro(pwm);
        motoresSetCorrecao(0);

        const int32_t eE = encoderLerEsquerdo();
        const int32_t eD = encoderLerDireito();

        // Marca o início da janela steady-state (depois da rampa).
        if (!janelaMarcada && agora - t0 >= JANELA_INI_MS) {
            eEsqJanela = eE;
            eDirJanela = eD;
            janelaMarcada = true;
        }

        if (nAmostras < MAX_AMOSTRAS) {
            Amostra& a = buffer[nAmostras++];
            a.t_ms   = agora - t0;
            a.encEsq = eE;   a.encDir = eD;
            a.pwmEsq = motorLerVelocidadeAtual(MOTOR_ESQUERDO);
            a.pwmDir = motorLerVelocidadeAtual(MOTOR_DIREITO);
        }
    }

    // Freio ativo pra parar seco.
    motoresFrear();
    const uint32_t tf = millis();
    while (millis() - tf < 200) { motoresAtualizar(); delay(2); }
    motoresParar();
    motoresAtualizar();

    // Totais e razão na janela steady-state (mais limpa que o total com rampa).
    const int32_t totEsq = encoderLerEsquerdo();
    const int32_t totDir = encoderLerDireito();
    const long janEsq = labs((long)totEsq - eEsqJanela);
    const long janDir = labs((long)totDir - eDirJanela);
    const float razao = (janEsq != 0) ? (float)janDir / (float)janEsq : 0.0f;

    if (comCSV) {
        logLinha(String("---- INICIO TELEMETRIA (CSV) [simetria_pwm") + pwm + "] ----");
        logLinha("t_ms,encEsq,encDir,pwmEsq,pwmDir");
        char linha[96];
        for (int i = 0; i < nAmostras; i++) {
            const Amostra& a = buffer[i];
            snprintf(linha, sizeof(linha), "%lu,%ld,%ld,%d,%d",
                     (unsigned long)a.t_ms, (long)a.encEsq, (long)a.encDir, a.pwmEsq, a.pwmDir);
            logLinha(String(linha));
        }
        logLinha(String("---- FIM (") + nAmostras + " amostras) ----");
    }

    logLinha(String("[SIM] PWM=") + pwm
             + " | total esq=" + totEsq + " dir=" + totDir
             + " | janela esq=" + janEsq + " dir=" + janDir
             + " | RAZAO dir/esq=" + String(razao, 3)
             + (abortou ? "  (ABORTADO)" : ""));
    logLinha(String("[SIM]   -> ") + (razao < 1.0f
             ? "direita anda MENOS (arrasto/mais fraca na direita)"
             : "direita anda MAIS (esquerda mais fraca/arrasto)"));
}

bool lerComando(char& c) {
    if (Serial.available() > 0) { c = (char)Serial.read(); return true; }
    if (client && client.connected() && client.available() > 0) { c = (char)client.read(); return true; }
    return false;
}

void logLinha(const String& s) {
    Serial.println(s);
    if (client && client.connected()) client.println(s);
}

void conectarWiFi() {
    Serial.printf("[SIM] Conectando em \"%s\" (DHCP) ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[SIM] Wi-Fi OK. Conecte em  %s:%u\n",
                      WiFi.localIP().toString().c_str(), WIFI_PORTA);
        server.begin();
    } else {
        Serial.println("\n[SIM] Wi-Fi FALHOU. Cheque SSID/senha.");
    }
}
