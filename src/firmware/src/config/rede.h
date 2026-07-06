#pragma once

// -----------------------------------------------------------------------------
// Configuração de rede da telemetria (Wi-Fi + WebSocket).
//
// PREENCHA WIFI_SSID/WIFI_PASS com o hotspot 2,4 GHz e WS_HOST com o IP do PC
// que roda o backend (o mesmo que aparece em `ip addr`/`ipconfig`, na mesma rede).
//
// O robô conecta como CLIENTE WebSocket em:
//   ws://<WS_HOST>:<WS_PORT><WS_PATH>
// e se identifica como "robo" (papel) pela query ?role=robo — é assim que o
// backend distingue o robô (produz telemetria) dos apps web (só visualizam).
// -----------------------------------------------------------------------------

#define WIFI_SSID "iPhone"          // <-- nome da rede/hotspot (2,4 GHz)
#define WIFI_PASS "06543210"        // <-- senha

#define WS_HOST   "192.168.0.100"   // <-- IP do PC rodando o backend
#define WS_PORT   3000
#define WS_PATH   "/ws?role=robo"

// Cadência de envio da telemetria ao vivo (~5 Hz). Um envio EXTRA e imediato é
// disparado ao entrar num estado terminal, para o backend finalizar a corrida
// sem esperar o próximo tick.
#define TELEMETRIA_INTERVALO_MS 200

// Timeout da conexão Wi-Fi inicial e intervalo de reconexão do WebSocket.
#define WIFI_TIMEOUT_MS         15000
#define WS_RECONNECT_MS         3000
