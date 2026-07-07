const INTERVALO_MS = Number(process.env.INTERVALO_MS ?? 1000);

let tempoCorrida = 0;

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
    sensores: { frente: 28, esquerda: 5, direita: 58 },
  },
  {
    x: 1,
    y: 3,
    direcao: "OESTE",
    sensores: { frente: 58, esquerda: 5, direita: 28 },
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
function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}
async function enviarTelemetria(indice, estadoRobo = "EXPLORANDO") {
  const ponto = ROTA[indice];

  const bateria = Math.max(
    0,
    100 - Math.floor((indice / (ROTA.length - 1)) * 10),
  );

  const tempoAtual = tempoCorrida;
  tempoCorrida += INTERVALO_MS;

  const payload = {
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
  };

  try {
    await fetch("http://localhost:3000/api/telemetria", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(payload),
    });

    console.log(
      `[${(tempoAtual / 1000).toFixed(1)}s]`,
      `[${estadoRobo}]`,
      `(${ponto.x}, ${ponto.y})`,
    );
  } catch (error) {
    console.error("Erro ao enviar:", error);
  }
}

async function iniciarSimulacao() {
  console.log("🚀 Iniciando corrida 4x4");

  for (let i = 0; i < ROTA.length; i++) {
    const estado = i === ROTA.length - 1 ? "OBJETIVO_ENCONTRADO" : "EXPLORANDO";

    enviarTelemetria(i, estado);

    if (i < ROTA.length - 1) {
      await sleep(INTERVALO_MS);
    }
  }

  console.log("\n🏁 Objetivo encontrado!");
  console.log(`⏱️ Tempo total: ${(tempoCorrida / 1000).toFixed(1)}s`);
}
iniciarSimulacao();
