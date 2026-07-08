# HANDOFF — Micromouse (Grupo 01, PI1) — Firmware

> Documento de transferência de contexto para continuar o trabalho num novo chat.
> Escrito em 2026-07-05. Entrega do projeto: **quarta-feira** (prazo curto).
> Leia tudo antes de mexer em qualquer coisa.

---

## ⚡ ATUALIZAÇÃO 2026-07-07 (LER PRIMEIRO — estado mais recente)

Muita coisa avançou depois de 05/07. Este bloco é o estado ATUAL; o resto do doc abaixo é o histórico/detalhe de 05/07.

**VALIDADO e funcionando (com bateria carregada):**
- **Odometria** revalidada no chassi novo: `CIRC=14.06`, `PPR=570` (45 cm real = 44,8 lido, <0,4%). Encoders simétricos (~1%).
- **IMU/giro:** deriva **~0,002 °/s**, calibra certo (comando `b`/`bc` no curva bench).
- **ToF:** sensores consistentes (direita crua infla **~+83 mm**; após a correção −50 fica igual à esquerda, offset de montagem ~+32 mm nos dois; `DIF_ALVO=−55` válido, centro real ~−49/−51). **Ressalva REAL:** a direita cospe **8190** (código de erro) esporádico → falta um filtro de sanidade.
- **Reta + centralização** OK. `VEL_MIN` subido **50→65** (atrito do labirinto lixado travava no fim).
- **Giro com VIÉS** validado: o giro para ANTES de 90° (`VIES_CURVA=6.5`) → resíduo cai **PRA FORA** (lado seguro); a navegação mira nos 90° reais e a reta seguinte completa (não corrompe a bússola).
- **CURVA DO ROBÔ GRANDE — Plano A COMPLETO E VALIDADO** (arquivo novo `teste_curva_planoA.cpp`): esquadro por toque → giro com viés → saída com centralização. `cd`/`ce` assentam **~1–2° de 90°, lado seguro, sem raspar**.

**DIAGNÓSTICO-CHAVE (o "enviesado pra direita"):** NÃO era giro, NÃO era ToF, NÃO era odometria. Era **BATERIA FRACA** — a queda de tensão sob carga (sag) **AMPLIFICA** uma assimetria minúscula de tração (~2%, medida no `teste_simetria`) para ~12%, empurrando a roda mais fraca abaixo do "joelho" do motor. **Com bateria carregada o robô é simétrico e anda reto.** ➜ **REGRA: testar/rodar SEMPRE com bateria > 60–70%.** Comportamento errático/intermitente → **suspeitar da BATERIA primeiro** (foi ela em vários episódios).

**FALTA:**
1. **Integrar no `navegacao.cpp`** (portar primitivas validadas; `TAMANHO_CELULA_CM→18`; corrigir sinal da centralização; carregar resíduo entre células).
2. **Corrida completa flood fill 4×4.**
3. Menores (não bloqueiam): **filtro do 8190** (ToF), "arrancada" da saída (cruzeiro rampa do zero → senta-e-pula ~300 ms), `nc` que trava (cliente meio-aberto exige reboot).

**Ferramentas novas:** `teste_curva_planoA` (curva completa; comandos `cd/ce`=manobra, `x`=esquadro, `d/e`=giro puro, `r`=repete8x, `b/bc`=drift IMU, `t`=ToF ao vivo, `<num>`=ajusta viés, `p`=aborta) e `teste_simetria` (razão de tração dir/esq, no ar e no chão + varredura de constância). Wi-Fi agora costuma ser **hotspot iPhone (IP 172.20.10.5)** — ajustar SSID/senha no topo do bench e ver IP no serial no boot.

**Pino ADC livre pra medir bateria por software (futuro):** `GPIO 36` ou `39` (ADC1, seguros com Wi-Fi) — precisa de divisor de tensão + calibração.

---

## 1. O QUE É O PROJETO

Robô **micromouse** que resolve um labirinto **4×4** por **flood fill**.
- MCU: **ESP32** (`esp32dev`), **PlatformIO** + Arduino framework.
- Diretório do firmware: `src/firmware/` (é a raiz do repo git).
- Navegação: flood fill + FSM em `src/main.cpp` (produção). Move célula a célula:
  lê paredes → recalcula distâncias → escolhe vizinho de menor distância → gira → anda.
- Equipe: **Renan** (usuário) + um amigo (o código do amigo tinha o flood fill embarcado).

### Geometria do labirinto (PADRÃO, confirmado medindo)
- **Passo (pitch) = 18 cm** — distância centro-a-centro de células = **quanto o robô anda por célula**.
- **Interior entre paredes laterais = 16,8 cm** (18 − 1,2 de parede).
- ⚠️ No código, `TAMANHO_CELULA_CM = 16.8` está **ERRADO** para distância de viagem — deveria ser **18** (o pitch). O 16,8 é a largura interna. **Corrigir na integração.**
- Um corredor de 4 células = 3 movimentos (o robô começa DENTRO da célula 0).

---

## 2. O ROBÔ (medidas físicas — IMPORTANTES)

- **Comprimento:** 13,4 cm.
- **Eixo das rodas → frente:** 10,9 cm ⇒ **overhang traseiro = 2,5 cm** (as rodas ficam BEM ATRÁS).
- **Largura:** rodas = **9,8 cm** (ponto mais largo, borda externa a borda externa); chassi = **7,8 cm** (mais fino que as rodas — o carro consegue **angular** mesmo com uma roda encostada).
- **Circunferência da roda (calibrada):** `CIRCUNFERENCIA_CM = 14.06`, `PPR = 570` (em `sensores/encoders.cpp`).
- É um robô **grande e comprido** ("trator/caminhão") — a geometria gera o problema de curva (seção 6).

### Consequência geométrica (crucial para a curva)
Numa junção **com parede à frente**, a frente comprida bate na parede **antes** do eixo chegar ao centro da junção → o eixo para **~2,5 cm atrás do centro**. Ao girar 90° (rotação sempre em torno do eixo traseiro), o robô termina **deslocado pro lado de dentro da curva**, e a roda traseira (9,8 cm, só ~3,5 cm de folga por lado no corredor) **raspa/entra na parede**.

---

## 3. HARDWARE / PINOS (`src/config/pinos.h`)

- **I2C:** SDA=21, SCL=22, 400 kHz. (IMU e ToF usam.)
- **IMU:** MPU6050 @ 0x68. Integra o **giro Z** pra heading (rumo). Range **±500°/s**.
- **ToF:** 3× VL53L0X. Índices: **0=frente, 1=esquerda, 2=direita**.
  - XSHUT {26, 4, 23}, endereços {0x30, 0x31, 0x32}.
- **Motores:** driver **TB6612FNG**. Canal A = **esquerdo** (AIN1=13, AIN2=25, PWMA=18); Canal B = **direito** (BIN1=14, BIN2=27, PWMB=19); STBY=5. PWM 20 kHz, 8 bits.
- **Encoders:** ESP32Encoder (attachFullQuad).
  - Esquerdo = **32/33** (GPIO normais, usam pull-up **interno**).
  - Direito = **34/35** (input-only, **SEM** pull-up interno → **pull-up EXTERNO no hardware**, já resolvido).
  - `ESP32Encoder::useInternalWeakPullResistors = puType::up` (correto no cenário misto).

---

## 4. ARQUITETURA DO FIRMWARE (camadas)

```
main.cpp                 -> FSM + flood fill (PRODUÇÃO; ainda NÃO validado end-to-end)
navegacao/
  flood_fill.{h,cpp}     -> BFS/flood fill (lógica OK; MAZE_N=4)
  navegacao.{h,cpp}      -> primitivas de movimento (navAndarUmaCelula, navGirar*) [WIP]
  pid.{h,cpp}            -> PID genérico (anti-windup condicional) [OK, reutilizado por tudo]
atuadores/
  motores.{h,cpp}        -> TB6612: cruzeiro+correção, rampa, e motoresFrear() (short-brake)
sensores/
  i2c_bus.{h,cpp}        -> Wire.begin nos pinos certos + i2cScan
  imu.{h,cpp}            -> MPU6050, integração do ângulo Z
  encoders.{h,cpp}       -> distância/pulsos
  tof.{h,cpp}            -> 3 ToF; tofLerDistancia (com correção) e tofLerDistanciaBruta (cru)
scripts_teste/           -> bancadas isoladas (um env por teste)
tools/captura_log.py     -> captura telemetria CSV (Wi-Fi TCP ou Serial)
```

**IMPORTANTE:** `navegacao.cpp` é um RASCUNHO (WIP). As primitivas validadas de verdade estão nos `scripts_teste` (reta, giro, centralização). A integração vai **portar** o que foi validado pra dentro do `navAndarUmaCelula`/`navGirarDelta`.

---

## 5. ESTADO DE VALIDAÇÃO (o que funciona / o que falta)

| Item | Status | Detalhe |
|---|---|---|
| Encoders / odometria | ✅ | CIRC=14,06 (empurrão de 50 cm reais = ~2027 pulsos) |
| IMU | ✅ | (teve episódio de sinal invertido — ver seção 7) |
| Motores | ✅ | + `motoresFrear()` (short-brake) adicionado |
| ToF (3 sensores) | ✅ com ressalva | direita infla ~+55 mm; correção em janela pode glitchar; dado falso esporádico |
| **Reta (heading-hold)** | ✅ | rumo < 0,5°, sem derrapagem |
| Reta: desaceleração + freio ativo | ✅ | pousa ±0,3–1 cm em 30/40/50 cm; comando de distância dinâmico |
| **Centralização por ToF (cascata)** | ✅ | posição→setpoint de rumo + deadband; validado isolado no `teste_centralizacao` (KP_POS=0.10, VIES_MAX=7, DEADBAND=10). Substituiu o `teste_reta_centro` (diferencial fraco, apagado). |
| Wi-Fi | ✅ | NÃO é fonte de problema (loop saudável); DHCP no roteador VIVOFIBRA |
| **Giro no MATERIAL (com viés)** | ✅ | resolvido com VIÉS: para antes de 90° → resíduo pra fora (lado seguro). `VIES_CURVA=6.5` |
| **Curva do robô grande (Plano A)** | ✅ | `teste_curva_planoA`: esquadro+giro+saída; `cd/ce` ~1–2° de 90°, sem raspar (bateria boa) |
| Esquadro na parede da frente | ✅ | esquadro POR TOQUE (stall dos encoders + creep travado + confirmação); esquadra rumo+posição |
| **Integração navegacao.cpp** | ❌ A FAZER | portar primitivos validados; corrigir bug de sinal da centralização lá; `TAMANHO_CELULA_CM→18` |
| Corrida completa flood fill 4×4 | ❌ A FAZER | objetivo final |

---

## 6. O PROBLEMA DA CURVA (robô grande) — decisões já tomadas

**Sintoma:** vira 90° numa junção, termina deslocado pro lado de dentro, roda traseira raspa (seção 2).

**Causa:** eixo traseiro não alcança o centro da junção quando há parede à frente.

**Só acontece com PAREDE À FRENTE** (junção fechada tipo L/T). Em cruzamento aberto o eixo chega ao centro e o giro é limpo. Como a navegação **já sabe** se tem parede à frente (leu o ToF pra decidir virar), o condicional é **de graça**.

**Plano acordado (incremental):**
- **Plano A (simples, tentar primeiro):** avançar **até a parede** (esquadro — minimiza o offset pro melhor caso ~2,5 cm, e corrige o rumo) → girar 90° limpo → sair andando **com a centralização** puxando pro meio. Cálculo: no melhor caso a roda fica a ~1 cm da parede; andando paralelo + centralização, descola sem raspar.
- **Plano B (se 1 cm raspar):** **manobra multi-ponto** — girar ~45° → andar pra frente na diagonal (anda o eixo pro centro do corredor) → girar os outros ~45° → (opcional) **dar ré** pra desfazer o avanço e não errar a contagem de células. É condicional a ter parede à frente.
- Alternativa descartada por complexidade/prazo: curva em arco.

---

## 7. DESCOBERTAS/DECISÕES IMPORTANTES (o "porquê" da história)

1. **"Problema de Wi-Fi" NÃO era Wi-Fi.** Loop 100% saudável no cabo E no Wi-Fi (teste_conexao + logs). Nem software.
2. **Era BROWNOUT** por uma **junta fria de solda num capacitor** (não a bateria — a bateria sempre foi boa). Refeita a solda, o robô na bateria = igual no USB. A troca de bateria só coincidiu com o cap soltar.
3. **IMU inverteu o sinal uma vez** (remontagem/afrouxamento físico da MPU6050) → o PID de rumo virou **realimentação positiva** → robô espiralou pra direita e travou (encoders diziam "direita", `ang_z` subia = "esquerda"). **Se a reta voltar a espiralar, checar o sinal do giro Z ANTES de tudo.** Usuário reajustou e voltou.
4. **Odometria nunca esteve errada** — o "erro de 20%" foi **medida errada do usuário** (mediu 40 cm, era 50). CIRC real ≈ 14,06 (≈ o 14,13 original).
5. **Coast/inércia ao parar** → resolvido com **perfil de desaceleração** (rampa a velocidade nos últimos cm) + **freio ativo** (`motoresFrear()` short-brake). Resultado: pouso milimétrico.
6. **Erro por-célula acumula** (4 corridas de 13,2 andam mais que 1 de 52,8) → resolver na navegação com **carregar o resíduo pra frente** + **esquadro/rezerar na parede**.
7. **teste_encoders_wifi** precisou de `motoresInit()+motoresHabilitar(false)` — sem isso os pinos do TB6612 flutuam e **travam uma roda** aleatoriamente (short-brake fantasma). Mesmo motivo de um push de calibração ter dado ruim antes.
8. **Centralização por ToF (validada):** usa o **CRU da direita** (`tofLerDistanciaBruta`) pra evitar o salto da janela de correção; alvo = **dif medido no centro (~−55)**, não 0 (absorve o viés dos sensores); ganho **suave**; só age com **parede dos dois lados**; sinal **corrigido** (o `navegacao.cpp` tem o sinal INVERTIDO — bug a corrigir lá na integração).
9. **Jam de largada:** se o robô começa **prensado e angulado contra a parede**, a roda trava e ele não anda. **Não é bloqueador** (na navegação real ele nunca começa assim — parte sempre ~centrado/reto). Fix planejado: **destravamento por ré** (se não largar em ~300 ms, dá ré curta). Fica pra integração.

---

## 8. PARÂMETROS CALIBRADOS (valores atuais)

**Odometria:** `CIRCUNFERENCIA_CM=14.06`, `PPR=570`.

**Reta (`teste_reta_imu` e `teste_centralizacao`):**
- `PWM_BASE=120`, PID rumo `KP=6, KI=1.5, KD=0.3`, `LIMITE_CORRECAO=70`, `LIMITE_INTEGRAL=40`.
- Desaceleração: `VEL_MIN=65` (subido de 50 — atrito novo do labirinto lixado travava no fim), `DECEL_CM=8`, freio `FREIO_MS=250`.

**Curva (`teste_curva_planoA`):**
- `VIES_CURVA=6.5` (giro para antes de 90° → resíduo pra fora).
- Esquadro: `PWM_APROX=100`, `PWM_TOQUE=55`, `TOF_CREEP_MM=120` (creep travado), `STALL_CONFIRM_MS=150` (empurrão a 100 confirma parede vs atrito).
- Saída: `PWM_SAIDA_BASE=120`, `DIST_SAIDA_CM=18`. Reusa centralização e rumo.
- PWM de operação no labirinto: ~90–100 (120 bate forte na parede em teste cru).

**Centralização em CASCATA (`teste_centralizacao`) — validado 07/07, substitui o antigo diferencial:**
- Laço externo: `erroPos = difC − DIF_ALVO` → viés no setpoint de rumo; laço interno = PID de rumo persegue.
- `KP_POS=0.10` (graus de viés por unidade de difC), `VIES_RUMO_MAX=7`, `DEADBAND_POS=10` (~6 mm, contínuo).
- `DIF_ALVO=-55` (= esq_corr − dir_cru no centro; medido ~−60). Gate por dist. CORRIGIDA `< 140` mm (não o cru 300, que ligava no aberto). Sanidade ToF: rejeita 0/8191/>2000.
- Comandos live: `kp`/`vm`/`db`. Baixar 0.20→0.10 e 12→7 derrubou o serpenteio pela metade (max|ang| 15,8°→6,8°).

**Giro (`teste_giro_stress`):**
- `KP=5, KI=8, KD=0.3`, `LIMITE_SAIDA=150`, `LIMITE_INTEGRAL=15`, `PWM_MIN_GIRO=90`.
- Conclusão: `TOL_ANGULO=4.5`, `TOL_VEL_GIRO=15`, fallback "parado".
- Desaceleração angular: `GRAUS_DECEL=40`, `VEL_GIRO_MIN=95`.
- Fechado no material com a estratégia do **VIÉS** (ver ATUALIZAÇÃO no topo): o giro para antes de 90° e a reta/saída seguinte completa vindo de fora. Não se persegue mais "acertar 90° na mosca".

**ToF direita:** crua infla **~+83 mm** (medido com régua no chassi novo); a correção −50 deixa a corrigida ~igual à esquerda (offset de montagem ~+32 mm nos dois lados). Pra centralização usa-se o CRU (`tofLerDistanciaBruta`) com `DIF_ALVO=−55`. **Cospe 8190 (erro) esporádico → falta filtro de sanidade.**

---

## 9. CONVENÇÕES (não errar os sinais)

- **Motor:** `correcao > 0` → curva pra **ESQUERDA** (roda direita mais rápida); `< 0` → **DIREITA**.
  - `vEsq = base − correcao`, `vDir = base + correcao`.
- **IMU:** ângulo **AUMENTA** = giro à **esquerda** (anti-horário); **DIMINUI** = **direita**. `navGirarDireita = −90`.
- **Flood fill (bússola):** NORTE=0, LESTE=1, SUL=2, OESTE=3. Girar à direita = +1 (mod 4) = IMU −90.
- **Centralização:** `dif = esq − dir`. Perto da esquerda → dif bem negativo → curva pra direita. Sinal certo: `ajuste = KP_CENTRO*(dif − DIF_ALVO)`. **(No `navegacao.cpp` está invertido — corrigir.)**
- **⚠️ Como o USUÁRIO reporta erro:** "**+X**" significa **OVERSHOOT** (passou do alvo), NÃO faltou. Vale pra giro e reta.

---

## 10. FERRAMENTAS E FLUXO DE TESTE

- **PlatformIO.** O binário `pio` NÃO está no PATH — usar `~/.platformio/penv/bin/pio`.
- **Gravar:** `pio run -e <env> -t upload`. **Sempre regravar ao trocar de teste** (o `--nome` da captura NÃO troca o firmware).
- **Ver IP / serial:** `pio device monitor -e <env>` (115200).
- **Captura de telemetria:** `python3 tools/captura_log.py --ip <IP> --nome <x> --comando <cmd>` (ou `--serial /dev/ttyACM0`). O `--comando` já manda com `\n`.
  - Reta/centro: `--comando 40` (distância cm) ou `s`.
  - Giro (`teste_giro_stress`): `1`=+90, `2`=−90, `3`=+180, `4`=−180, `5`=stress(10 giros), `6`=sonda de torque.
  - Testes de fluxo contínuo (tof_wifi, encoders_wifi, conexao): usar `nc <IP> 8080` (não têm marcadores INICIO/FIM).
- **Wi-Fi:** agora **DHCP** no roteador **VIVOFIBRA-8681** (o IP sai no serial no boot). Antes era hotspot iPhone (IP estático 172.20.10.5).
  - ⚠️ `teste_encoders_wifi` ainda está com IP estático da faixa do iPhone (172.20.10.x) → **não conecta no roteador**; trocar pra DHCP se for usar.

### Envs existentes (`platformio.ini`)
`producao`, `teste_motores`, `teste_imu`, `teste_reta_imu`, **`teste_centralizacao`** (reta + centralização em cascata — substituiu o `teste_reta_centro`), `teste_conexao`, `teste_giro_stress`, `teste_encoders_wifi`, `teste_tof_wifi`, `teste_giro_imu`, **`teste_curva_planoA`** (curva completa — o mais importante), **`teste_simetria`** (razão de tração dir/esq).
- **Apagados:** `teste_encoders` e `teste_tof` (seriais).
- **Diagnóstico embutido no `teste_curva_planoA`:** `b`/`bc` (deriva do IMU), `t` (monitor ToF ao vivo, corrigido+cru dos 3), `x` (só esquadro, loga a aproximação).
- **`teste_simetria`:** manda PWM igual nos 2 motores e mede razão dir/esq (rodar no AR e no CHÃO; `r` varre 80/100/120 pra ver se a assimetria é constante).

---

## 11. PRÓXIMOS PASSOS (roadmap, em ordem) — ATUALIZADO 07/07

✅ **FEITO:** giro no material (com viés), esquadro por toque, **curva do robô grande (Plano A completo e validado)**. Ver bloco de ATUALIZAÇÃO no topo.

0. **Sempre testar/rodar com bateria > 60–70%** (a maioria dos "bugs" intermitentes era sag de bateria; ver ATUALIZAÇÃO).
0.5. **REVALIDAR a curva ANTES de integrar** (passo imediato, decisão do usuário 07/07): com a **bateria carregada**, rodar vários `cd`/`ce` no `teste_curva_planoA`, gerar logs e confirmar que está tudo consistente (sem viés, sem raspar, sem reset). Só avançar pra integração depois disso estar limpo e sem nada incomodando.
1. **Integrar no `navegacao.cpp`** (`navAndarUmaCelula` / `navGirarDelta`) — o grande passo que falta:
   - Portar as primitivas validadas do `teste_curva_planoA` (esquadro+giro c/ viés+saída centralizada) e da reta (desaceleração+freio).
   - **Corrigir o sinal da centralização** (o do `navegacao.cpp` está INVERTIDO).
   - Corrigir `TAMANHO_CELULA_CM` → **18** (é o pitch).
   - **Carregar o resíduo** entre células; desligar centralização sem parede dos 2 lados.
2. **Filtro de sanidade no ToF** (rejeitar 8190/valores fora de faixa) — importante pra leitura de parede na navegação.
3. **Corrida completa no 4×4** (flood fill).
4. **Plano B de emergência:** seguidor de parede (regra da mão direita) resolve muitos 4×4 se a integração empacar no prazo.
5. **Menores (não bloqueiam):** "arrancada" da saída (começar cruzeiro em ~70 em vez de rampar do 0); `nc` que trava (fazer cliente novo assumir); medidor de bateria por software (GPIO 36/39).

---

## 12. COMO TRABALHAR COM O RENAN (preferências)

- **Pedir aprovação ANTES de alterar código/arquivo** (não depois).
- **Incremental, passo a passo:** construir uma peça, **testar e validar isolada**, entender o que está sendo feito antes de seguir.
- Prefere **criar arquivo de teste novo** a modificar os já validados.
- Gosta de **entender o "porquê"** (explicar conceitos, não só entregar código).
- Reporta erro "+X" como **overshoot** (seção 9).
- Idioma: **português**.
- Há memórias persistentes no projeto: `pedir-antes-de-alterar`, `firmware-estado-validacao`, `convencao-erro-overshoot` (o novo chat carrega o `MEMORY.md` automaticamente).

---

## 13. ARQUIVOS-CHAVE PRA OLHAR PRIMEIRO (novo chat)

- `HANDOFF.md` (este — LER o bloco de ATUALIZAÇÃO 07/07 no topo primeiro).
- `platformio.ini` (envs).
- **`src/scripts_teste/teste_curva_planoA.cpp`** (curva completa Plano A — o mais recente e completo; esquadro+giro c/ viés+saída; + diagnósticos `b/bc/t`).
- `src/scripts_teste/teste_centralizacao.cpp` (reta + centralização em cascata — substituiu o `teste_reta_centro`).
- `src/scripts_teste/teste_simetria.cpp` (razão de tração dir/esq).
- `src/atuadores/motores.cpp` (cruzeiro+correção, rampa, `motoresFrear`).
- `src/sensores/tof.cpp` (correção da direita + `tofLerDistanciaBruta`; **falta filtro do 8190**).
- `src/navegacao/navegacao.cpp` (WIP; alvo da integração; **tem o bug de sinal da centralização**).
- `tools/captura_log.py` (captura de logs).
