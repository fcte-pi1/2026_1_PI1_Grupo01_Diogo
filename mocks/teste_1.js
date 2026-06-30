const INTERVALO_MS = Number(process.env.INTERVALO_MS ?? 1000);

let tempoCorrida = 0;

const ROTA = [
  {
    x: 0,
    y: 0,
    direcao: "LESTE",
    sensores: { frente: 30, esquerda: 0, direita: 0 },
  },
  {
    x: 1,
    y: 0,
    direcao: "LESTE",
    sensores: { frente: 20, esquerda: 0, direita: 0 },
  },
  {
    x: 2,
    y: 0,
    direcao: "LESTE",
    sensores: { frente: 5, esquerda: 0, direita: 20 },
  },
  {
    x: 2,
    y: 1,
    direcao: "SUL",
    sensores: { frente: 20, esquerda: 5, direita: 5 },
  },
  {
    x: 1,
    y: 1,
    direcao: "OESTE",
    sensores: { frente: 5, esquerda: 20, direita: 5 },
  },
  {
    x: 1,
    y: 2,
    direcao: "SUL",
    sensores: { frente: 20, esquerda: 5, direita: 5 },
  },
  {
    x: 2,
    y: 2,
    direcao: "LESTE",
    sensores: { frente: 0, esquerda: 0, direita: 0 },
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

  const payload = {
    tempo_corrida_ms: tempoCorrida,
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
    const response = await fetch("http://localhost:3000/api/telemetria", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(payload),
    });

    const data = await response.json();

    console.log(
      `[${(tempoCorrida / 1000).toFixed(1)}s]`,
      `[${estadoRobo}]`,
      `(${ponto.x}, ${ponto.y})`,
      data.id ?? "OK",
    );
  } catch (error) {
    console.error("Erro ao enviar:", error);
  }

  tempoCorrida += INTERVALO_MS;
}

async function iniciarSimulacao() {
  console.log("🚀 Iniciando corrida 4x4");

  for (let i = 0; i < ROTA.length - 1; i++) {
    await enviarTelemetria(i, "EXPLORANDO");
    await sleep(INTERVALO_MS);
  }

  await enviarTelemetria(ROTA.length - 1, "OBJETIVO_ENCONTRADO");

  console.log("");
  console.log("🏁 Objetivo encontrado!");
  console.log("📍 Centro: (2,2)");
  console.log(`⏱️ Tempo total: ${(tempoCorrida / 1000).toFixed(1)}s`);
}

iniciarSimulacao();
