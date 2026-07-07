import { PrismaClient } from "@prisma/client";
const prisma = new PrismaClient();

// Estados terminais enviados pelo robô na telemetria → status final da corrida.
// É o robô quem decide o fim da corrida e se ela foi concluída ou não.
const ESTADOS_FINAIS: Record<string, string> = {
  OBJETIVO_ENCONTRADO: "CONCLUIDA",
  CONCLUIDO: "CONCLUIDA",
  ERRO: "NAO_CONCLUIDA",
};

export const TelemetryService = {
  // Remove uma corrida e suas telemetrias (cascade definido no schema).
  async deleteRun(id: string) {
    return prisma.run.delete({ where: { id } });
  },

  async save(data: any) {
    let runId: string | undefined = data.runId;

    if (runId) {
      // O robô (ou um teste) enviou um runId explícito: garante que a corrida exista.
      const runExists = await prisma.run.findUnique({ where: { id: runId } });
      if (!runExists) {
        await prisma.run.create({ data: { id: runId } });
      }
    } else {
      // Sem runId: anexa à corrida ativa (EM_ANDAMENTO) mais recente.
      // Assim a telemetria do robô entra na corrida que o usuário iniciou pela web.
      const corridaAtiva = await prisma.run.findFirst({
        where: { status: "EM_ANDAMENTO" },
        orderBy: { startedAt: "desc" },
      });

      if (corridaAtiva) {
        runId = corridaAtiva.id;
      } else {
        // Nenhuma corrida ativa: cria uma automaticamente para não perder o dado.
        const novaCorrida = await prisma.run.create({ data: {} });
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
    // O guard "status: EM_ANDAMENTO" evita sobrescrever uma corrida já encerrada.
    const statusFinal = ESTADOS_FINAIS[estadoRobo];
    if (statusFinal) {
      await prisma.run.updateMany({
        where: { id: runId, status: "EM_ANDAMENTO" },
        data: { status: statusFinal, endedAt: new Date() },
      });
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