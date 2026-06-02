import { PrismaClient } from "@prisma/client";
const prisma = new PrismaClient();

export const TelemetryService = {
  async save(data: any) {
    let runId = data.runId || "corrida-padrao";

    // Busca se a corrida enviada pelo robô já existe no banco
    const runExists = await prisma.run.findUnique({ where: { id: runId } });

    // Se ela não existir, nós criamos ela com o ID que o robô pediu!
    if (!runExists) {
      await prisma.run.create({ data: { id: runId } });
    }
    
    return prisma.telemetry.create({
      data: {
        runId: runId,
        batteryLevel: Number(data.batteryLevel),
        positionX: Number(data.positionX),
        positionY: Number(data.positionY),
        linearVelocity: Number(data.linearVelocity)
      }
    });
  },

  async getLatest() {
    return prisma.telemetry.findFirst({ orderBy: { timestamp: "desc" } });
  }
};