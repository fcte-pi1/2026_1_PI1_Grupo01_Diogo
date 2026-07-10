# Escopo — Conexão WebSocket no Firmware + Telemetria em Tempo Real + Salvamento da Corrida

> Objetivo: o robô se conecta ao backend por **WebSocket**, transmite telemetria
> em tempo real (visualizada na web) e a corrida é **salva no banco** ao terminar
> — encerrada **automaticamente** quando o robô conclui (chega ao centro / trava)
> **ou manualmente** pelo botão "Parar" na web.

---

## 1. Decisões tomadas (âncoras deste escopo)

| # | Decisão | Escolha | Consequência |
|---|---------|---------|--------------|
| D1 | Modelo de controle | **Robô só envia (send-only)** | Firmware navega autônomo (RF01) e apenas transmite. O backend/web controlam o **registro** da corrida. **Sem canal de comando web→robô.** |
| D2 | Status ao encerrar manualmente | **`NAO_CONCLUIDA`** | Só vira `CONCLUIDA` quando o robô realmente chega ao centro (estado terminal `OBJETIVO_ENCONTRADO`). |
| D3 | Bateria | **Placeholder por agora** | Firmware envia `bateria_pct` fixo (ex.: 100). INA219 real fica para tarefa futura. |

---

## 2. Estado atual (o que já existe)

### Backend (nesta branch — WebSocket completo)
- `api/ws/realtime.ts`: `WebSocketServer` em `/ws`, distinção de papel `?role=robo` vs `app`, envelope `{type, payload}`, heartbeat ping/pong. No `type: "telemetria"` **valida campos obrigatórios** (`tempo_corrida_ms`, `posicao_x`, `posicao_y`, `bateria_pct`) → chama `TelemetryService.save()` → **faz broadcast** para os clients `app`.
- `api/ws/protocol.ts`: `envelope()`, `parseMensagem()` (tolerante: objeto cru vira telemetria), `parsePapel()`.
- `api/services/telemetry.service.ts`:
  - `save()`: sem `runId` → anexa à corrida **`EM_ANDAMENTO`** mais recente; se não houver, **cria uma**. Mapeia campos e, ao receber um **estado terminal**, finaliza a corrida:
    ```ts
    ESTADOS_FINAIS = { OBJETIVO_ENCONTRADO:"CONCLUIDA", CONCLUIDO:"CONCLUIDA", ERRO:"NAO_CONCLUIDA" }
    ```
    (guard `status:"EM_ANDAMENTO"` evita sobrescrever corrida já encerrada).
  - **Não existe** `finalizeRun`/`endRun` — o fim manual ainda não é suportado.
- Schema Prisma: `Run { id, status(default EM_ANDAMENTO), startedAt, endedAt }`, `Telemetry { ...contrato... }`, cascade on delete.

### Frontend (nesta branch)
- `lib/run-context.tsx`: `CorridaProvider` consome o WS como `app`, entende o envelope, faz **backfill** via REST (`/runs/:id/telemetries`) e anexa pontos ao vivo. "Parar" hoje só **para de observar** (não finaliza o registro no banco).

### Firmware (branch `feat/arquitetura-fsm-imu` — navegação pronta, **sem WebSocket**)
- `src/firmware/src/main.cpp`: laço Flood Fill funcional. Estado disponível: `robX, robY, robDir (NORTE/LESTE/SUL/OESTE)`, `concluido`, distâncias ToF, ângulo IMU, encoders.
  - Terminais já existem: `OBJETIVO ALCANCADO` e `Preso: nenhuma direção válida`.
- Primitivas de navegação (`navegacao.h`) são **BLOQUEANTES** (rodam PID interno chamando `imuAtualizar()`/`motoresAtualizar()`).
- Comunicação de rede: **só protótipos** em `scripts_teste/` (`teste_conexao.cpp`, `teste_tof_wifi.cpp`…) usando **WiFiServer TCP porta 8080 + CSV**. Não há cliente WebSocket.
- `platformio.ini`: já inclui `bblanchon/ArduinoJson`; **falta** biblioteca de WebSocket client.
- **Alerta comprovado nos testes**: fazer I/O de rede **dentro do laço de controle bloqueante trava a navegação** (é exatamente o que `teste_conexao.cpp` foi feito para diagnosticar).

---

## 3. Arquitetura da solução

### 3.1 Firmware — dois núcleos (FreeRTOS)
O ESP32 é dual-core. Separar navegação de rede resolve o problema de travamento:

- **Core 1 — Navegação** (`loop()` do Arduino, como hoje): Flood Fill + primitivas PID bloqueantes. **Não toca em rede.**
- **Core 0 — Telemetria/WebSocket** (task FreeRTOS `xTaskCreatePinnedToCore`, junto da pilha WiFi): mantém a conexão WS viva (`webSocket.loop()`), envia telemetria a **cadência fixa ~5 Hz** e reconecta sozinho.

**Estado compartilhado** (único ponto sensível): um `struct` *snapshot* protegido por **mutex** (`portMUX`/`SemaphoreHandle_t`):
```
struct SnapshotTelemetria {
  uint32_t tempo_ms;
  uint8_t  x, y;
  Direcao  dir;
  const char* estado;         // "EXPLORANDO" | "OBJETIVO_ENCONTRADO" | "ERRO"
  uint16_t frente_mm, esq_mm, dir_mm;
};
```
A navegação **escreve** o snapshot a cada célula/transição; a task de telemetria **lê** (com lock) e serializa em JSON. Envio **imediato** ao mudar para estado terminal (não espera o tick de 5 Hz), para o backend finalizar a corrida sem atraso.

### 3.2 Ciclo de vida da corrida (send-only)
```
Web "Iniciar"  → só começa a OBSERVAR (abre WS como app). NÃO cria corrida.
Robô 1ª telemetria (sem runId) → backend cria/anexa a corrida EM_ANDAMENTO.
   (mantém "Iniciar = observar" para não criar corrida concorrente — evita o
    bug de "aparecer a última corrida" já corrigido antes.)
Robô chega ao centro → envia estado OBJETIVO_ENCONTRADO → backend finaliza CONCLUIDA.
Robô trava        → envia estado ERRO → backend finaliza NAO_CONCLUIDA.
Web "Parar" (manual) → chama endpoint de finalização → backend marca NAO_CONCLUIDA.
```

### 3.3 Contrato de telemetria (firmware → backend)
Envelope: `{"type":"telemetria","payload":{...}}` para `ws://<IP_DO_PC>:3000/ws?role=robo`.
**Não enviar `runId`** (deixa o backend anexar à corrida ativa).

| Campo payload | Origem no firmware | Observação |
|---|---|---|
| `tempo_corrida_ms` | `millis() - t0` | t0 = início da navegação |
| `posicao_x` / `posicao_y` | `robX` / `robY` | |
| `direcao_atual` | `robDir` → string | `NORTE/LESTE/SUL/OESTE` |
| `estado_robo` | mapeado | `EXPLORANDO` (normal), `OBJETIVO_ENCONTRADO` (centro), `ERRO` (preso) |
| `bateria_pct` | **placeholder** (ex.: 100) | D3 — obrigatório pelo validador |
| `leitura_sensores.dist_frente_cm` | `tofLerDistancia(0)/10` | mm→cm |
| `leitura_sensores.dist_esquerda_cm` | `tofLerDistancia(1)/10` | mm→cm |
| `leitura_sensores.dist_direita_cm` | `tofLerDistancia(2)/10` | mm→cm |

> As strings de `estado_robo` **devem** bater exatamente com as chaves de `ESTADOS_FINAIS` no backend.

---

## 4. Mudanças por camada

### 4.1 Firmware — branch nova a partir de `feat/arquitetura-fsm-imu`
- **`platformio.ini`**: adicionar `links2004/WebSockets` (`WebSocketsClient`) ao `lib_deps`; novo env `teste_ws`.
- **`src/config/rede.h`** (novo): `WIFI_SSID`, `WIFI_PASS`, `WS_HOST`, `WS_PORT=3000`, `WS_PATH="/ws?role=robo"`, `TELEMETRIA_HZ=5`. (Centraliza as credenciais que hoje estão espalhadas nos testes.)
- **`src/comunicacao/telemetria.{h,cpp}`** (novo):
  - `telemetriaInit()` — conecta WiFi + WS client, sobe a task no Core 0.
  - `telemetriaAtualizarSnapshot(...)` — API chamada pela navegação (grava snapshot sob mutex).
  - Task interna: `webSocket.loop()`, envio a 5 Hz + envio imediato em estado terminal, reconexão.
  - Serialização com ArduinoJson no formato do contrato (§3.3).
- **`src/main.cpp`**: `telemetriaInit()` no `setup()`; chamar `telemetriaAtualizarSnapshot()` a cada célula/transição e nos ramos terminais (`OBJETIVO_ENCONTRADO`, `ERRO`). **Nenhuma** chamada de rede direta no laço.
- **`src/scripts_teste/teste_ws.cpp`** (novo): streama telemetria falsa para o backend rodando (espelha o `simulate-websocket.ts`), validando a ponta firmware→backend **antes** de mexer no `main.cpp`.

### 4.2 Backend — branch nova a partir desta branch
- **`telemetry.service.ts`**: novo método
  ```ts
  async finalizeRun(id: string, status = "NAO_CONCLUIDA") {
    return prisma.run.updateMany({
      where: { id, status: "EM_ANDAMENTO" },
      data: { status, endedAt: new Date() },
    });
  }
  ```
  (mesmo guard do auto-encerramento; idempotente).
- **Gatilho de finalização manual** — endpoint REST (request/response, fácil de testar no Jest):
  `PATCH /api/telemetria/runs/:id/finalizar` → `finalizeRun(id, "NAO_CONCLUIDA")`.
  Adicionar controller + rota.
- **Testes**: casos no `TelemetryService`/controller (finaliza EM_ANDAMENTO; não sobrescreve corrida já CONCLUIDA; 404/no-op se id inexistente).

### 4.3 Frontend — mesma branch do backend
- `lib/run-context.tsx`: no "Parar", se houver `runIdAtual`, chamar `PATCH /runs/:id/finalizar` **antes** de parar de observar (mantém o comportamento de preservar o último caminho na tela).
- Ajuste/observar exibição do status `NAO_CONCLUIDA` para parada manual (já suportado pela tabela de corridas).

---

## 5. Arquivos (resumo)

**Criar**
- `src/firmware/src/config/rede.h`
- `src/firmware/src/comunicacao/telemetria.h` / `.cpp`
- `src/firmware/src/scripts_teste/teste_ws.cpp`
- `src/backend/api/controllers/…` rota `finalizar` (+ testes)

**Modificar**
- `src/firmware/platformio.ini` (lib + env `teste_ws`)
- `src/firmware/src/main.cpp` (init + snapshots)
- `src/backend/api/services/telemetry.service.ts` (`finalizeRun`)
- `src/backend/api/routes/telemetry.routes.ts`
- `src/frontend/lib/run-context.tsx` (Parar → finalizar)

---

## 6. Plano de testes / integração local
1. **Backend** `npm run dev` (porta 3000). Descobrir o IP do PC na rede local.
2. **Firmware isolado**: env `teste_ws` → confirma conexão WS + telemetria chegando (log do backend, aparece no frontend).
3. **Firmware real**: env `producao` com `telemetriaInit()` no `main.cpp`; robô no labirinto → telemetria ao vivo no frontend, corrida `CONCLUIDA` ao chegar ao centro.
4. **Fim manual**: durante a corrida, "Parar" na web → corrida vira `NAO_CONCLUIDA` no banco.
5. Backend: `npm test` (novos testes de `finalizeRun`). Frontend: Vitest.

> Firmware e web rodam em **branches diferentes** (firmware ← `feat/arquitetura-fsm-imu`; backend+frontend ← esta branch). Para testar juntos, subir o backend desta branch e apontar o firmware para o IP do PC.

---

## 7. Riscos
- **Starvation do `webSocket.loop()`** pelas primitivas bloqueantes → mitigado pela task no Core 0 (§3.1). É o ponto que mais exige atenção.
- **Corrida no snapshot compartilhado** → mutex obrigatório; snapshot pequeno e copiado sob lock.
- **Corrida fantasma** (telemetria antes de "Iniciar" cria run) → manter "Iniciar = observar" e run criado na 1ª telemetria; não criar run concorrente na web.
- **Wi-Fi instável** (visto nos testes: quedas/stalls) → reconexão automática na task; `WiFi.setSleep(false)`.

## 8. Fora de escopo (tarefas futuras)
- Integração real do **INA219** (bateria) — hoje placeholder (D3).
- **Comando web→robô** (start/stop remoto) — descartado por D1 (RF01).
- Cálculo/persistência de `velocidade_media`, `tempo_conclusao` agregados e `trajeto_coordenadas` consolidado no `Run` (o MER do relatório prevê; hoje derivado das telemetrias).
