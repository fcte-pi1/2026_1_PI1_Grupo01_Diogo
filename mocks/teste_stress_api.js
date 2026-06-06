const GRID_SIZE = 8;
const INTERVALO_MS = 1;
const DURACAO_SEGUNDOS = 90;

let tempoCorrida = 0;

let posicao = {
  x: 0,
  y: 0,
};

let direcaoAtual = "LESTE";

const direcoes = [
  { nome: "NORTE", dx: 0, dy: -1 },
  { nome: "SUL", dx: 0, dy: 1 },
  { nome: "LESTE", dx: 1, dy: 0 },
  { nome: "OESTE", dx: -1, dy: 0 },
];

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function escolherMovimento() {
  const possibilidades = direcoes.filter(dir => {
    const novoX = posicao.x + dir.dx;
    const novoY = posicao.y + dir.dy;

    return (
      novoX >= 0 &&
      novoX < GRID_SIZE &&
      novoY >= 0 &&
      novoY < GRID_SIZE
    );
  });

  return possibilidades[
    Math.floor(Math.random() * possibilidades.length)
  ];
}

async function enviarTelemetria(estadoRobo = "EXPLORANDO") {
  if (estadoRobo === "EXPLORANDO") {
    const movimento = escolherMovimento();

    posicao.x += movimento.dx;
    posicao.y += movimento.dy;

    direcaoAtual = movimento.nome;
  }

  const bateria = Math.max(
    0,
    100 - Math.floor(tempoCorrida / 5000)
  );

  const payload = {
    tempo_corrida_ms: tempoCorrida,
    posicao_x: posicao.x,
    posicao_y: posicao.y,
    direcao_atual: direcaoAtual,
    estado_robo: estadoRobo,
    bateria_pct: bateria,

    dist_frente_cm: +(Math.random() * 30).toFixed(2),
    dist_esquerda_cm: +(Math.random() * 30).toFixed(2),
    dist_direita_cm: +(Math.random() * 30).toFixed(2),
  };

  try {
    const response = await fetch(
      "http://localhost:3000/api/telemetria",
      {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify(payload),
      }
    );

    const data = await response.json();

    console.log(
      `[${tempoCorrida / 1000}s]`,
      `[${estadoRobo}]`,
      `(${payload.posicao_x}, ${payload.posicao_y})`,
      data
    );
  } catch (error) {
    console.error("Erro ao enviar:", error);
  }

  tempoCorrida += INTERVALO_MS;
}

async function iniciarSimulacao() {
  console.log("🚀 Iniciando corrida...");

  // Exploração normal
  for (let i = 0; i < DURACAO_SEGUNDOS - 1; i++) {
    await enviarTelemetria("EXPLORANDO");
    await sleep(INTERVALO_MS);
  }

  // Última telemetria: encontrou o objetivo
  await enviarTelemetria("OBJETIVO_ENCONTRADO");

  console.log("");
  console.log("🏁 CENTRO DO LABIRINTO ENCONTRADO!");
  console.log(
    `📍 Posição final: (${posicao.x}, ${posicao.y})`
  );
  console.log(
    `⏱️ Tempo total: ${(tempoCorrida / 1000).toFixed(1)}s`
  );
  console.log("✅ Corrida finalizada.");
}

iniciarSimulacao();