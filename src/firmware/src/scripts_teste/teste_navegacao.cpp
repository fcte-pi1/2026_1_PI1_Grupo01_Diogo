#include <Arduino.h>
#include <WiFi.h>
#include "config/pinos.h"
#include "sensores/i2c_bus.h"
#include "sensores/imu.h"
#include "sensores/tof.h"
#include "sensores/encoders.h"
#include "atuadores/motores.h"
#include "navegacao/navegacao.h"

// =============================================================================
// BANCADA DA NAVEGAÇÃO (Inc. 1) — valida navAndarUmaCelula() ISOLADO
//
// Exercita a PRIMITIVA de produção do navegacao.cpp (reta com centralização em
// cascata + desaceleração + freio ativo), SEM a lógica de decisão do flood fill.
// Serve pra confirmar que o port ficou bom: cada célula anda ~o pitch, o rumo se
// mantém (bússola persiste entre células) e ele centraliza + pousa suave.
//
// COMO USAR (nc <IP> 8080 ou Serial):
//   <num>  -> anda N células em sequência (ex.: 3). Loga distância e rumo de CADA célula.
//   a      -> anda 1 célula.
//   s      -> repete o último N.
//   z      -> rezera a bússola de rumo (navZerarRumo) no rumo atual.
//
// IMPORTANTE: mantenha o robô 100% PARADO durante "Calibrando IMU" no boot
// (calibração ruim do offset do giro faz o rumo derivar -> a reta sai torta).
// A primitiva é BLOQUEANTE (não dá pra abortar no meio de uma célula).
// =============================================================================

const char*    WIFI_SSID       = "Vilbs";
const char*    WIFI_PASS       = "12345678";
const uint16_t WIFI_PORTA      = 8080;
const uint32_t WIFI_TIMEOUT_MS = 8000;
WiFiServer server(WIFI_PORTA);
WiFiClient client;

int ultimoN = 1;

// Índices dos ToF (confirmados no HANDOFF §3: 0=frente, 1=esquerda, 2=direita).
static const int TOF_FRENTE = 0, TOF_ESQUERDA = 1, TOF_DIREITA = 2;

void      conectarWiFi();
bool      lerComando(char& c);
void      logLinha(const String& s);
void      andarNCelulas(int n);
void      girarELogar(char dir);
void      curvaCompleta(char dir);
void      paredesLog();

void setup() {
    Serial.begin(115200);
    delay(200);

    i2cInit();
    if (!imuInit()) {
        Serial.println("[NAVTEST] ERRO: IMU. Travado.");
        while (true) delay(1000);
    }
    Serial.println("[NAVTEST] Calibrando IMU — mantenha o robo PARADO...");
    imuCalibrarOffsetZ(500);
    imuZerarAnguloZ();

    if (!tofInit())     { Serial.println("[NAVTEST] ERRO: ToF. Travado.");     while (true) delay(1000); }
    if (!motoresInit()) { Serial.println("[NAVTEST] ERRO: Motores. Travado."); while (true) delay(1000); }
    if (!navInit())     { Serial.println("[NAVTEST] ERRO: Nav. Travado.");     while (true) delay(1000); }

    navZerarRumo();   // bússola de referência = rumo atual (0 após zerar o IMU)

    conectarWiFi();
    logLinha("[NAVTEST] Pronto. <num>=anda | a=1cel | cd/ce=curva | x=esq | w=paredes | r=re | d/e=gira | v=vies | kp/vm/db=centro | s=repete | z=rezera");
}

void loop() {
    if (server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            logLinha("=== Conectado a bancada de navegacao (Inc.1) ===");
            logLinha("<num>=anda | a=1cel | cd/ce=curva | x=esq | w=paredes | r=re | d/e=gira | v=vies | kp/vm/db=centro | s=repete | z=rezera");
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
        String low = bufCmd; low.toLowerCase();

        if (low == "a") {
            andarNCelulas(1);
        } else if (low == "cd") {
            curvaCompleta('d');
        } else if (low == "ce") {
            curvaCompleta('e');
        } else if (low == "x") {
            const bool tocou = navEsquadrar();
            const float d = 0.5f * (encoderDistanciaEsquerdaCm() + encoderDistanciaDireitaCm());
            logLinha(String("[NAVTEST] esquadro: ") + (tocou ? "TOCOU" : "nao tocou")
                     + ", andou " + String(d, 1) + " cm | rumo=" + String(imuLerAnguloZ(), 2) + " deg");
        } else if (low == "w") {
            paredesLog();
        } else if (low == "r") {
            logLinha("[NAVTEST] Andando 1 celula de RE...");
            navAndarUmaCelula(true);
            const float d = fabsf(0.5f * (encoderDistanciaEsquerdaCm() + encoderDistanciaDireitaCm()));
            logLinha(String("[NAVTEST] re: recuou ") + String(d, 1) + " cm | rumo=" + String(imuLerAnguloZ(), 2) + " deg");
        } else if (low == "d") {
            girarELogar('d');
        } else if (low == "e") {
            girarELogar('e');
        } else if (low.startsWith("kp")) {
            const float v = bufCmd.substring(2).toFloat();
            navDefinirCentroKp(v);
            logLinha(String("[NAVTEST] KP_POS (centro) = ") + String(v, 3));
        } else if (low.startsWith("vm")) {   // ANTES de "v" (vm começa com v)
            const float v = bufCmd.substring(2).toFloat();
            navDefinirCentroViesMax(v);
            logLinha(String("[NAVTEST] VIES_RUMO_MAX (centro) = ") + String(v, 1) + " graus");
        } else if (low.startsWith("db")) {
            const float v = bufCmd.substring(2).toFloat();
            navDefinirCentroDeadband(v);
            logLinha(String("[NAVTEST] DEADBAND (centro) = ") + String(v, 1) + " (unid. difC)");
        } else if (low.startsWith("v")) {
            const float vv = bufCmd.substring(1).toFloat();
            navDefinirViesCurva(vv);
            logLinha(String("[NAVTEST] VIES_CURVA (giro) ajustado para ") + String(vv, 1) + " graus");
        } else if (low == "s") {
            andarNCelulas(ultimoN);
        } else if (low == "z") {
            navZerarRumo();
            logLinha("[NAVTEST] Rumo rezerado no rumo atual.");
        } else {
            int n = bufCmd.toInt();
            if (n > 0 && n <= 20) { ultimoN = n; andarNCelulas(n); }
        }
        bufCmd = "";
    }
    delay(5);
}

// Anda N células em sequência, logando a distância e o rumo de CADA uma.
// (Após navAndarUmaCelula() retornar, os encoders ainda guardam os pulsos daquela
//  célula — ela zera no INÍCIO —, então dá pra ler a distância percorrida.)
void andarNCelulas(int n) {
    logLinha(String("[NAVTEST] Andando ") + n + " celula(s)...");
    float total = 0.0f;
    for (int k = 1; k <= n; k++) {
        navAndarUmaCelula();

        const float dCel = 0.5f * (encoderDistanciaEsquerdaCm() + encoderDistanciaDireitaCm());
        total += dCel;
        logLinha(String("[NAVTEST]  celula ") + k + "/" + n
                 + ": andou " + String(dCel, 1) + " cm"
                 + " | rumo=" + String(imuLerAnguloZ(), 2) + " deg");
        delay(250);   // assenta entre células
    }
    logLinha(String("[NAVTEST] Fim. Total ~") + String(total, 1)
             + " cm (esperado ~" + String(n * 17.6f, 1) + "). Meça com a regua e compare.");
}

// Gira 90° (d=direita/-90, e=esquerda/+90) e loga quanto girou + o resíduo.
// LEMBRE: o giro para ANTES de 90 (viés) DE PROPÓSITO -> resíduo POSITIVO = parou
// antes = PRA FORA (bom, lado seguro). A reta seguinte completa o resto do rumo.
void girarELogar(char dir) {
    // Referência LIMPA por giro (zera IMU + rumo): mede a folga real da volta,
    // sem o acúmulo bússola-vs-físico de giros encadeados sem reta no meio.
    imuZerarAnguloZ();
    navZerarRumo();
    if (dir == 'd') navGirarDireita(); else navGirarEsquerda();
    const float girou   = imuLerAnguloZ();       // rotação a partir do 0
    const float residuo = 90.0f - fabsf(girou);
    logLinha(String("[NAVTEST] giro ") + dir + ": girou " + String(girou, 2)
             + " deg | residuo=" + String(residuo, 2)
             + (residuo >= 0.0f ? " (PRA FORA, bom)" : " (PRA DENTRO, ruim)"));
}

// Curva completa (Plano A) na direção dir: esquadro -> giro c/ viés -> anda 1 célula
// (a "saída" que completa o resíduo + centraliza). Espelha o orientarPara + andar do
// main.cpp. Precisa de parede À FRENTE e corredor aberto na direção do giro.
void curvaCompleta(char dir) {
    logLinha("[NAVTEST] Curva completa (Plano A): esquadro -> giro -> anda...");
    const bool tocou = navEsquadrar();
    if (!tocou) { logLinha("[NAVTEST] NAO encostou -> curva CANCELADA."); return; }
    delay(200);
    if (dir == 'd') navGirarDireita(); else navGirarEsquerda();
    delay(200);
    navAndarUmaCelula();
    logLinha(String("[NAVTEST] Curva '") + dir + "' concluida. rumo final="
             + String(imuLerAnguloZ(), 2) + " deg");
}

// Valida a PERCEPÇÃO de parede (filtro do Inc 5a) com o robô PARADO num cruzamento.
// 1) Varredura CRUA (instantânea) dos 3 ToF por ~1,5 s -> você vê o glitch da direita
//    ao vivo (ela cospe 0/8190 esporádico).
// 2) DECISÃO filtrada (navParedeFrente/Esquerda/Direita), que usa a mediana de
//    amostras válidas -> deve ficar ESTÁVEL e certa mesmo se a leitura crua piscar.
void paredesLog() {
    logLinha("[NAVTEST] Varredura CRUA (10x) - frente | esq_corr | dir_corr | dir_bruta:");
    for (int i = 0; i < 10; i++) {
        const uint16_t f  = tofLerDistancia(TOF_FRENTE);
        const uint16_t e  = tofLerDistancia(TOF_ESQUERDA);
        const uint16_t d  = tofLerDistancia(TOF_DIREITA);
        const uint16_t db = tofLerDistanciaBruta(TOF_DIREITA);
        logLinha(String("   f=") + f + "  e=" + e + "  d=" + d + "  d_bruta=" + db + " mm");
        delay(120);
    }
    const bool pf = navParedeFrente();
    const bool pe = navParedeEsquerda();
    const bool pd = navParedeDireita();
    logLinha(String("[NAVTEST] DECISAO (filtrada): frente=") + (pf ? "PAREDE" : "livre")
             + " | esq=" + (pe ? "PAREDE" : "livre")
             + " | dir=" + (pd ? "PAREDE" : "livre"));
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
    Serial.printf("[NAVTEST] Conectando em \"%s\" (DHCP) ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[NAVTEST] Wi-Fi OK. Conecte em  %s:%u\n",
                      WiFi.localIP().toString().c_str(), WIFI_PORTA);
        server.begin();
    } else {
        Serial.println("\n[NAVTEST] Wi-Fi FALHOU. Cheque SSID/senha e se a rede e 2,4GHz.");
    }
}
