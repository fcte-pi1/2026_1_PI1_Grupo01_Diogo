import { WebSocket } from "ws";

// Conecta como o ROBÔ (role=robo): só envia telemetria, não recebe o eco.
const ws = new WebSocket("ws://localhost:3000/ws?role=robo");

const INTERVALO_MS = Number(process.env.INTERVALO_MS ?? 1000);
const RUN_ID = `sim-4x4-${Date.now()}`;

let tempoCorrida = 0;
let indice = 0;
let intervalId: ReturnType<typeof setInterval> | undefined;

// Circuito 4x4: sai de (0,0), dá a volta pela borda e faz um desvio pelo
// miolo do labirinto antes de terminar em (1,2).
const ROTA = [
  {
    x: 0,
    y: 0,
    direcao: "LESTE",
    sensores: { frente: 88, esquerda: 5, direita: 88 },
  },
  {
    x: 1,
    y: 0,
    direcao: "LESTE",
    sensores: { frente: 58, esquerda: 5, direita: 28 },
  },
  {
    x: 2,
    y: 0,
    direcao: "LESTE",
    sensores: { frente: 28, esquerda: 5, direita: 58 },
  },
  {
    x: 3,
    y: 0,
    direcao: "LESTE",
    sensores: { frente: 5, esquerda: 5, direita: 88 },
  },
  {
    x: 3,
    y: 1,
    direcao: "SUL",
    sensores: { frente: 58, esquerda: 5, direita: 28 },
  },
  {
    x: 3,
    y: 2,
    direcao: "SUL",
    sensores: { frente: 28, esquerda: 5, direita: 58 },
  },
  {
    x: 3,
    y: 3,
    direcao: "SUL",
    sensores: { frente: 5, esquerda: 5, direita: 88 },
  },
  {
    x: 2,
    y: 3,
    direcao: "OESTE",
    sensores: { frente: 28, esquerda: 58, direita: 5 },
  },
  {
    x: 1,
    y: 3,
    direcao: "OESTE",
    sensores: { frente: 58, esquerda: 28, direita: 5 },
  },
  {
    x: 0,
    y: 3,
    direcao: "OESTE",
    sensores: { frente: 5, esquerda: 5, direita: 88 },
  },
  {
    x: 0,
    y: 2,
    direcao: "NORTE",
    sensores: { frente: 28, esquerda: 58, direita: 5 },
  },
  {
    x: 0,
    y: 1,
    direcao: "NORTE",
    sensores: { frente: 58, esquerda: 28, direita: 5 },
  },
  {
    x: 1,
    y: 1,
    direcao: "LESTE",
    sensores: { frente: 28, esquerda: 28, direita: 28 },
  },
  {
    x: 2,
    y: 1,
    direcao: "LESTE",
    sensores: { frente: 5, esquerda: 58, direita: 28 },
  },
  {
    x: 2,
    y: 2,
    direcao: "SUL",
    sensores: { frente: 5, esquerda: 28, direita: 58 },
  },
  {
    x: 1,
    y: 2,
    direcao: "OESTE",
    sensores: { frente: 5, esquerda: 58, direita: 28 },
  },
];

function montarPayload(i: number, estadoRobo: string) {
  const ponto = ROTA[i];
  const bateria = Math.max(0, 100 - Math.floor((i / (ROTA.length - 1)) * 10));

  const tempoAtual = tempoCorrida;
  tempoCorrida += INTERVALO_MS;

  return {
    tempo_corrida_ms: tempoAtual,
    posicao_x: ponto.x,
    posicao_y: ponto.y,
    direcao_atual: ponto.direcao,
    estado_robo: estadoRobo,
    bateria_pct: bateria,
    leitura_sensores: {
      dist_frente_cm: ponto.sensores.frente,
      dist_esquerda_cm: ponto.sensores.esquerda,
      dist_direita_cm: ponto.sensores.direita,
    },
    runId: RUN_ID,
  };
}

ws.on("open", () => {
  console.log("Cliente WebSocket conectado. Iniciando corrida 4x4...");

  intervalId = setInterval(() => {
    const ultimoPonto = indice === ROTA.length - 1;
    const estadoRobo = ultimoPonto ? "OBJETIVO_ENCONTRADO" : "EXPLORANDO";

    const payload = montarPayload(indice, estadoRobo);

    // Envia no envelope padrão { type, payload }.
    ws.send(JSON.stringify({ type: "telemetria", payload }));
    console.log(
      `[${(payload.tempo_corrida_ms / 1000).toFixed(1)}s] [${estadoRobo}] (${payload.posicao_x}, ${payload.posicao_y})`,
    );

    indice += 1;

    if (ultimoPonto) {
      clearInterval(intervalId);
      ws.close();
      console.log("\n🏁 Objetivo encontrado!");
      console.log(`⏱️ Tempo total: ${(tempoCorrida / 1000).toFixed(1)}s`);
    }
  }, INTERVALO_MS);
});

ws.on("message", (data) => {
  console.log("Mensagem recebida do servidor:", data.toString());
});

ws.on("error", (error) => {
  console.error("Erro no WebSocket:", error);
});

ws.on("close", () => {
  console.log("Conexão encerrada.");
});
