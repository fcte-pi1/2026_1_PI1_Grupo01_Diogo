# README (temporário) — Testar tudo junto (E2E)

> Guia rápido para rodar **firmware → backend → frontend** e ver a telemetria ao
> vivo. Documento temporário — some quando o E2E entrar no fluxo oficial.
> Detalhes de arquitetura/decisões em `docs/escopo_websocket_firmware.md`.

## Visão geral

```
  ESP32 (firmware)                      PC (backend + frontend)
  ┌──────────────────┐   Wi-Fi 2,4 GHz  ┌──────────────────────────────┐
  │ Navegação Core 1 │───── ws://──────▶ │ backend  :3000  (WS + REST)  │
  │ Telemetria Core 0│  ?role=robo       │ frontend :3001 (dashboard)   │
  └──────────────────┘                   └──────────────────────────────┘
```

Regra de ouro: **ESP e PC na MESMA rede Wi-Fi 2,4 GHz**, e `WS_HOST` = **IP do PC**.

## Pré-requisitos

- **Node 20+** e **npm**
- **PlatformIO** (`pio`) — CLI ou extensão do VS Code
- Cabo **de dados** USB para a ESP + hotspot/roteador **2,4 GHz**

## 1. Backend (porta 3000)

```bash
cd src/backend
npm install
npx prisma generate
npx prisma migrate dev        # 1ª vez: cria o dev.db
npm run dev                   # "Servidor online: http://localhost:3000"
```

### 1.1 Teste do WebSocket sem firmware

Se você quiser validar só o sistema web/backend, use o simulador de WebSocket:

```bash
cd src/backend
npx tsx api/simulate-websocket.ts
```

Ele conecta como `role=robo`, envia telemetria falsa e fecha a corrida ao final.
Com isso, o frontend só precisa estar aberto em `http://localhost:3001` para observar.

## 2. Frontend (porta 3001)

```bash
cd src/frontend
npm install
npm run dev                   # http://localhost:3001
```

Abra **http://localhost:3001** e clique em **Iniciar** (começa a observar o WebSocket).
No painel da corrida, o tamanho do labirinto escolhido na UI é o valor usado para a corrida atual e para o que fica salvo no banco.

## 3. Descobrir o IP do PC (vai na ESP)

- macOS: `ipconfig getifaddr en0`
- Linux: `hostname -I | awk '{print $1}'`
- Windows: `ipconfig` → "Endereço IPv4" do adaptador Wi-Fi

## 4. Configurar a ESP — `src/firmware/src/config/rede.h`

```c
#define WIFI_SSID "SUA_WIFI"      // MESMA rede do PC, 2,4 GHz
#define WIFI_PASS "SUA_SENHA"
#define WS_HOST   "192.168.x.x"   // IP do PC (passo 3)  — NÃO o IP da ESP
```

## 5. Gravar a ESP

```bash
cd src/firmware
# bancada isolada (telemetria falsa, valida o E2E sem o robô navegar):
pio run -e teste_ws -t upload -t monitor
# depois, navegação real:
pio run -e producao -t upload -t monitor
```

Porta manual, se preciso: `--upload-port <COMx | /dev/cu.* | /dev/ttyUSB0>`.

## 6. O que confirma o E2E

| Onde                      | Esperado                                                                                  |
| ------------------------- | ----------------------------------------------------------------------------------------- |
| Serial da ESP             | `Wi-Fi OK IP=192.168.x.x` (mesma faixa do PC) → `conectado ao backend` → pacotes enviados |
| Backend                   | telemetria chegando                                                                       |
| Frontend (localhost:3001) | painel + minimapa ao vivo; corrida vira **CONCLUIDA** no fim                              |

## Teste sem placa (contrato do firmware)

```bash
cd src/firmware && pio test -e native     # 8/8, roda no PC
```

## Testes automatizados

```bash
cd src/backend  && npm test               # Jest
cd src/frontend && npm run test           # Vitest
```

## Teste WebSocket sem firmware

```bash
cd src/backend && npx tsx api/simulate-websocket.ts
```

Esse é o script que valida o fluxo web/backend sem depender do ratinho físico.

## Ver o que foi salvo (opcional)

```bash
cd src/backend && npx prisma studio       # tabelas runs / telemetries
```

---

## Troubleshooting (os erros que mais aparecem)

| Sintoma                                    | Causa                                                                            | Solução                                                                                             |
| ------------------------------------------ | -------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------- |
| `host unreachable` / `desconectado` na ESP | ESP e PC em **redes diferentes**, ou isolamento de clientes do Wi-Fi             | mesma Wi-Fi 2,4 GHz; `WS_HOST` = IP do PC; desligar "AP isolation"; testar `ping <IP_da_ESP>` do PC |
| ESP com IP de outra faixa                  | entrou na Wi-Fi errada                                                           | ajustar `WIFI_SSID` para a rede do PC                                                               |
| `Cannot GET /` ao abrir `http://IP:3000`   | **normal** — o backend respondeu (rota `/` não existe); prova que está acessível | —                                                                                                   |
| Frontend não carrega via IP                | resolvido: host derivado em runtime (`lib/backend.ts`)                           | garantir que abriu pelo IP certo                                                                    |
| ESP não vira porta serial (LED aceso)      | cabo só-carga ou placa USB-C sem resistores CC                                   | usar **cabo de dados** e, se USB-C, **cabo USB-A→USB-C**                                            |
| `Wi-Fi FALHOU`                             | rede 5 GHz                                                                       | usar 2,4 GHz                                                                                        |
