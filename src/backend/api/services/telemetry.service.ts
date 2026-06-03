import { PrismaClient } from "@prisma/client";
const prisma = new PrismaClient();

export const TelemetryService = {
  async save(data: any) {
    // Mantemos a lógica da corrida padrão caso o robô não envie um ID específico
    let runId = data.runId || "corrida-padrao";

    // Busca se a corrida enviada pelo robô já existe no banco
    const runExists = await prisma.run.findUnique({ where: { id: runId } });

    // Se ela não existir, nós criamos ela com o ID que o robô pediu
    if (!runExists) {
      await prisma.run.create({ data: { id: runId } });
    }
    
    // Extraímos o objeto de sensores com uma proteção (fallback) caso venha vazio
    const sensores = data.leitura_sensores || {};

    return prisma.telemetry.create({
      data: {
        runId: runId,
        tempoCorridaMs: Math.floor(Number(data.tempo_corrida_ms)),
        posicaoX: Math.floor(Number(data.posicao_x)),
        posicaoY: Math.floor(Number(data.posicao_y)),
        direcaoAtual: String(data.direcao_atual || "NORTE"),
        estadoRobo: String(data.estado_robo || "EXPLORANDO"),
        bateriaPct: Math.floor(Number(data.bateria_pct)),
        
        // Mapeando as distâncias dos sensores
        distFrenteCm: Number(sensores.dist_frente_cm || 0),
        distEsquerdaCm: Number(sensores.dist_esquerda_cm || 0),
        distDireitaCm: Number(sensores.dist_direita_cm || 0)
      }
    });
  },

  async getLatest() {
    return prisma.telemetry.findFirst({ orderBy: { timestamp: "desc" } });
  }
};