import { PrismaClient } from "@prisma/client";
const prisma = new PrismaClient();

// Estados terminais enviados pelo robô na telemetria → status final da corrida.
// É o robô quem decide o fim da corrida e se ela foi concluída ou não.
const ESTADOS_FINAIS: Record<string, string> = {
  OBJETIVO_ENCONTRADO: "CONCLUIDA",
  CONCLUIDO: "CONCLUIDA",
  ERRO: "NAO_CONCLUIDA",
};

// Tolerância para detectar reset do robô (G2): se a última telemetria da
// corrida ativa tiver tempo_corrida_ms maior que o recebido por mais do que
// essa margem, o robô reiniciou a contagem (rebootou/reiniciou a corrida).
const TOLERANCIA_RESET_MS = 500;

// Monta os dados iniciais de uma corrida nova a partir do payload recebido.
// Hoje só aproveita o tamanho do labirinto (G5), quando o firmware o envia.
function dadosNovaCorrida(data: any): Record<string, unknown> {
  const dadosCorrida: Record<string, unknown> = {};
  if (data.tamanho_labirinto !== undefined && data.tamanho_labirinto !== null) {
    dadosCorrida.tamanhoLabirinto = Math.floor(Number(data.tamanho_labirinto));
  }
  return dadosCorrida;
}

// Calcula os agregados da corrida (G6) a partir das telemetrias já salvas,
// na ordem crescente de tempo_corrida_ms. Usado tanto no encerramento
// automático (robô manda estado terminal) quanto no manual (botão "Parar").
function calcularAgregados(telemetries: any[], status: string) {
  if (!telemetries || telemetries.length === 0) {
    return {
      tempoConclusaoMs: null,
      velocidadeMedia: null,
      consumoBateriaPct: null,
      desafioCumprido: status === "CONCLUIDA",
      trajetoCoordenadas: null,
    };
  }

  const primeiro = telemetries[0];
  const ultimo = telemetries[telemetries.length - 1];

  const tempoConclusaoMs = ultimo.tempoCorridaMs;
  const trajetoCoordenadas = JSON.stringify(
    telemetries.map((t) => ({ x: t.posicaoX, y: t.posicaoY }))
  );
  const consumoBateriaPct = Math.max(0, primeiro.bateriaPct - ultimo.bateriaPct);
  const velocidadeMedia =
    tempoConclusaoMs > 0 ? telemetries.length / (tempoConclusaoMs / 1000) : 0;
  const desafioCumprido = status === "CONCLUIDA";

  return {
    tempoConclusaoMs,
    velocidadeMedia,
    consumoBateriaPct,
    desafioCumprido,
    trajetoCoordenadas,
  };
}

export const TelemetryService = {
  // Remove uma corrida e suas telemetrias (cascade definido no schema).
  async deleteRun(id: string) {
    return prisma.run.delete({ where: { id } });
  },

  // Finaliza uma corrida com o status informado, calculando os agregados
  // (G6) a partir das telemetrias já salvas. Idempotente: o guard
  // "status: EM_ANDAMENTO" garante que uma corrida já finalizada não é
  // sobrescrita (no-op quando o id não existe ou já está finalizado).
  async finalizeRun(id: string, status: string = "NAO_CONCLUIDA") {
    const telemetries = await prisma.telemetry.findMany({
      where: { runId: id },
      orderBy: { tempoCorridaMs: "asc" },
    });

    const agregados = calcularAgregados(telemetries, status);

    return prisma.run.updateMany({
      where: { id, status: "EM_ANDAMENTO" },
      data: { status, endedAt: new Date(), ...agregados },
    });
  },

  async save(data: any) {
    let runId: string | undefined = data.runId;

    if (runId) {
      // O robô (ou um teste) enviou um runId explícito: garante que a corrida exista.
      const runExists = await prisma.run.findUnique({ where: { id: runId } });
      if (!runExists) {
        await prisma.run.create({ data: { id: runId, ...dadosNovaCorrida(data) } });
      }
    } else {
      // Sem runId: anexa à corrida ativa (EM_ANDAMENTO) mais recente.
      // Assim a telemetria do robô entra na corrida que o usuário iniciou pela web.
      const corridaAtiva = await prisma.run.findFirst({
        where: { status: "EM_ANDAMENTO" },
        orderBy: { startedAt: "desc" },
      });

      if (corridaAtiva) {
        // G2: detecta reset do robô. Se a última telemetria da corrida ativa
        // tiver tempo_corrida_ms maior que o recebido agora (além da
        // tolerância), o robô reiniciou no meio da corrida — fecha a corrida
        // órfã e abre uma nova em vez de misturar as duas tentativas.
        const ultimaTelemetria = await prisma.telemetry.findFirst({
          where: { runId: corridaAtiva.id },
          orderBy: { tempoCorridaMs: "desc" },
        });

        const tempoRecebido = Math.floor(Number(data.tempo_corrida_ms));
        const pareceReset =
          !!ultimaTelemetria &&
          ultimaTelemetria.tempoCorridaMs - tempoRecebido > TOLERANCIA_RESET_MS;

        if (pareceReset) {
          await TelemetryService.finalizeRun(corridaAtiva.id, "NAO_CONCLUIDA");
          const novaCorrida = await prisma.run.create({ data: dadosNovaCorrida(data) });
          runId = novaCorrida.id;
        } else {
          runId = corridaAtiva.id;
        }
      } else {
        // Nenhuma corrida ativa: cria uma automaticamente para não perder o dado.
        const novaCorrida = await prisma.run.create({ data: dadosNovaCorrida(data) });
        runId = novaCorrida.id;
      }
    }

    // Aceita os sensores tanto aninhados (contrato) quanto no nível raiz do payload.
    const sensores = data.leitura_sensores || data;
    const estadoRobo = String(data.estado_robo || "EXPLORANDO");

    const telemetria = await prisma.telemetry.create({
      data: {
        runId: runId,
        tempoCorridaMs: Math.floor(Number(data.tempo_corrida_ms)),
        posicaoX: Math.floor(Number(data.posicao_x)),
        posicaoY: Math.floor(Number(data.posicao_y)),
        direcaoAtual: String(data.direcao_atual || "NORTE"),
        estadoRobo: estadoRobo,
        bateriaPct: Math.floor(Number(data.bateria_pct)),

        // Mapeando as distâncias dos sensores
        distFrenteCm: Number(sensores.dist_frente_cm || 0),
        distEsquerdaCm: Number(sensores.dist_esquerda_cm || 0),
        distDireitaCm: Number(sensores.dist_direita_cm || 0)
      }
    });

    // O robô controla o fim da corrida: ao receber um estado terminal,
    // finaliza a corrida com o status correspondente (completou ou não).
    // finalizeRun já tem o guard "status: EM_ANDAMENTO" (idempotente).
    const statusFinal = ESTADOS_FINAIS[estadoRobo];
    if (statusFinal) {
      await TelemetryService.finalizeRun(runId, statusFinal);
    }

    return telemetria;
  },

  async getLatest() {
    return prisma.telemetry.findFirst({ orderBy: { timestamp: "desc" } });
  },

  async getRuns() {
    return prisma.run.findMany({
      orderBy: {
        startedAt: "desc",
      },
    });
  },

  async getRunById(id: string) {
    return prisma.run.findUnique({
      where: { id },
    });
  },

  async getTelemetriesByRunId(runId: string) {
    return prisma.telemetry.findMany({
      where: {
        runId,
      },
      orderBy: {
        tempoCorridaMs: "asc",
      },
    });
  },
};
