# Plano de execução — fechar os gaps do E2E (telemetria/WebSocket)

Estado atual: o caminho **firmware → backend → frontend** funciona (telemetria ao
vivo + salvamento com encerramento **automático**). Nesta rodada, o PR fecha os
gaps G1, G2, G4, G5 e G6; o **G3 segue aberto** para a próxima iteração, porque
depende de revisão fina da navegação para não emitir `ERRO` cedo demais. Este
plano lista o que ainda falta para o E2E ficar **completo e correto** com a
navegação real, em ordem de execução.

Legenda de camada: 🤖 firmware · 🖥️ backend · 🌐 frontend.

---

## G1 — Encerramento manual da corrida (botão "Parar") 🖥️🌐 · Prioridade: ALTA

**O quê:** hoje o "Parar" no frontend só para de observar; a corrida continua
`EM_ANDAMENTO` no banco. Falta finalizá-la manualmente.
**Por quê:** fecha o ciclo de salvamento — sem isso, uma corrida interrompida
nunca recebe status final e polui o histórico.
**Como:**

1. 🖥️ `TelemetryService.finalizeRun(id, status = "NAO_CONCLUIDA")` — `updateMany`
   com guard `status: "EM_ANDAMENTO"` (idempotente, não sobrescreve corrida já finalizada).
2. 🖥️ Rota `PATCH /api/telemetria/runs/:id/finalizar` (controller + rota).
3. 🌐 No "Parar" (`run-context`), se houver `runIdAtual`, chamar o endpoint **antes**
   de parar de observar (mantém o último caminho na tela).
4. 🖥️ Testes: finaliza `EM_ANDAMENTO`; não mexe em `CONCLUIDA`; no-op se id inexistente.
   **Arquivos:** `src/backend/api/services/telemetry.service.ts`, `.../controllers/*`,
   `.../routes/telemetry.routes.ts`, `src/frontend/lib/run-context.tsx`.
   **Aceite:** clicar "Parar" durante uma corrida → status vira `NAO_CONCLUIDA` no banco.

## G2 — Nova corrida por sessão (não misturar tentativas) 🤖🖥️ · Prioridade: ALTA

**O quê:** o robô não envia `runId`; o backend anexa à corrida `EM_ANDAMENTO` mais
recente. Se a corrida anterior não foi finalizada (robô resetou no meio), a
telemetria nova **gruda na corrida velha**.
**Por quê:** duas tentativas viram uma só → dados corrompidos.
**Como (escolher uma):**

- 🤖 Robô gera um `runId` novo a cada boot/partida e o envia; OU
- 🖥️ No `save()`, se `tempo_corrida_ms` recebido for **menor** que o último da corrida
  ativa (indício de reset), fechar a órfã (`NAO_CONCLUIDA`) e abrir uma nova.
  **Arquivos:** `src/firmware/src/comunicacao/telemetria*`, `main.cpp`;
  `src/backend/api/services/telemetry.service.ts`.
  **Aceite:** resetar o robô no meio de uma corrida gera **duas** corridas separadas.

## G3 — Estados terminais disciplinados 🤖 · Prioridade: MÉDIA

**O quê:** garantir que só a **falha/objetivo definitivos** emitam `ERRO` /
`OBJETIVO_ENCONTRADO`. A navegação real tem recuperação de travamento, ré e
re-tentativas — nenhum estado transitório pode disparar terminal.
**Por quê:** um `ERRO` transitório finaliza a corrida cedo demais como `NAO_CONCLUIDA`.
**Como:** revisar os pontos do `main.cpp`/navegação que chamam
`telemetriaAtualizar(..., "ERRO")`; usar `ERRO` só no beco sem saída real, não em
recuperações. Estados intermediários continuam como `EXPLORANDO`.
**Arquivos:** `src/firmware/src/main.cpp`, `navegacao/*`.
**Aceite:** o robô se recupera de um travamento sem a corrida ser finalizada.

## G4 — Bateria real via INA219 🤖 · Prioridade: MÉDIA

**O quê:** `bateria_pct` é placeholder `100` (o INA219 não está no firmware).
**Por quê:** telemetria de energia real (RF de bateria/consumo) e requisito do backend.
**Como:** add `adafruit/Adafruit INA219` ao `platformio.ini`; módulo
`sensores/energia.{h,cpp}` (lê tensão/corrente no I²C já existente); converter tensão
do pack 2S → `%`; alimentar o snapshot no lugar do `100`. O ponto de troca já está
isolado (`telemetria.cpp` / `telemetria_contrato`).
**Arquivos:** `platformio.ini`, `src/firmware/src/sensores/energia*`,
`comunicacao/telemetria.cpp`.
**Aceite:** `bateria_pct` reflete a carga real e cai ao longo da corrida.

## G5 — Tamanho do labirinto + orientação 🤖🌐 · Prioridade: MÉDIA

**O quê:** o firmware usa `MAZE_N`; o minimapa/entidade `Labirinto` têm 4x4/8x8/16x16.
A telemetria manda `posicao_x/y` mas **não** o tamanho. Além disso, a convenção do
robô (origem inferior-esquerda, NORTE inicial) precisa bater com o render do minimapa.
**Por quê:** sem o tamanho, o minimapa renderiza na escala errada; sem alinhar a
orientação, o caminho aparece espelhado/rotacionado.
**Como:** enviar a dimensão do labirinto na telemetria (ou fixar por corrida no web);
validar visualmente um percurso conhecido e ajustar o mapeamento de eixos no minimapa.
**Arquivos:** contrato (`telemetria_contrato`), `src/frontend/components/runs/minimap.tsx`.
**Aceite:** um percurso real aparece no minimapa na escala e orientação corretas.

## G6 — Métricas agregadas da corrida 🖥️ · Prioridade: BAIXA

**O quê:** o MER prevê no `Run`: `velocidade_media`, `tempo_conclusao`,
`consumo_bateria`, `desafio_cumprido`, `trajeto_coordenadas`. O schema atual só tem
`status/startedAt/endedAt`.
**Por quê:** relatório e telas de comparação esperam esses agregados.
**Como:** ao finalizar a corrida (auto ou manual), calcular a partir das telemetrias
e persistir no `Run` (migração Prisma + cálculo no service).
**Arquivos:** `prisma/schema.prisma`, `telemetry.service.ts`.
**Aceite:** ao finalizar, o `Run` traz os agregados; a tela de histórico os exibe.

---

## Ordem sugerida

1. **G1** (fecha o salvamento manual) → 2. **G2** (integridade das corridas) →
2. **G3** (não finalizar cedo) → 4. **G4** (bateria real) → 5. **G5** (minimapa correto)
   → 6. **G6** (métricas/relatório).

G1 e G2 são os que mais destravam o E2E "de produção"; G4–G6 são incrementos de
qualidade. Cada gap deve virar uma issue/PR próprio a partir da `feat/arquitetura-fsm-imu`.
