// ============================================================
// Testes Unitários — TelemetryService
// Valida a lógica de negócio do serviço de telemetria de forma
// completamente isolada do banco de dados real (Prisma mockado).
// ============================================================

import { prismaMock } from "../__mocks__/prisma.mock";

// Importa o serviço DEPOIS do mock para garantir que o Prisma
// já está interceptado pelo moduleNameMapper do Jest.
import { TelemetryService } from "../../services/telemetry.service";

// --- Payload de exemplo alinhado com o contrato da API ---
const payloadValido = {
  tempo_corrida_ms: 15400,
  posicao_x: 3,
  posicao_y: 4,
  direcao_atual: "NORTE",
  estado_robo: "EXPLORANDO",
  bateria_pct: 82,
  leitura_sensores: {
    dist_frente_cm: 12.5,
    dist_esquerda_cm: 4.1,
    dist_direita_cm: 15.0,
  },
};

// Registro que o Prisma devolveria após um create bem-sucedido
const registroCriado = {
  id: "uuid-telemetria-1",
  runId: "corrida-padrao",
  tempoCorridaMs: 15400,
  posicaoX: 3,
  posicaoY: 4,
  direcaoAtual: "NORTE",
  estadoRobo: "EXPLORANDO",
  bateriaPct: 82,
  distFrenteCm: 12.5,
  distEsquerdaCm: 4.1,
  distDireitaCm: 15.0,
  timestamp: new Date("2026-06-07T10:00:00.000Z"),
};

// ============================================================
describe("TelemetryService.save()", () => {
  // ----------------------------------------------------------
  it("deve criar uma nova corrida quando o runId não existe no banco", async () => {
    // Simula: corrida não encontrada → precisa criar
    prismaMock.run.findUnique.mockResolvedValue(null);
    prismaMock.run.create.mockResolvedValue({ id: "corrida-padrao" });
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    const resultado = await TelemetryService.save(payloadValido);

    expect(prismaMock.run.findUnique).toHaveBeenCalledWith({
      where: { id: "corrida-padrao" },
    });
    expect(prismaMock.run.create).toHaveBeenCalledWith({
      data: { id: "corrida-padrao" },
    });
    expect(resultado).toEqual(registroCriado);
  });

  // ----------------------------------------------------------
  it("não deve criar corrida duplicada quando ela já existe", async () => {
    // Simula: corrida encontrada → não cria de novo
    prismaMock.run.findUnique.mockResolvedValue({ id: "corrida-padrao" });
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save(payloadValido);

    expect(prismaMock.run.create).not.toHaveBeenCalled();
  });

  // ----------------------------------------------------------
  it("deve usar o runId customizado enviado pelo robô quando fornecido", async () => {
    const payloadComRunId = { ...payloadValido, runId: "corrida-123" };
    prismaMock.run.findUnique.mockResolvedValue(null);
    prismaMock.run.create.mockResolvedValue({ id: "corrida-123" });
    prismaMock.telemetry.create.mockResolvedValue({
      ...registroCriado,
      runId: "corrida-123",
    });

    const resultado = await TelemetryService.save(payloadComRunId);

    expect(prismaMock.run.findUnique).toHaveBeenCalledWith({
      where: { id: "corrida-123" },
    });
    expect(resultado.runId).toBe("corrida-123");
  });

  // ----------------------------------------------------------
  it("deve mapear corretamente todos os campos do payload para o banco", async () => {
    prismaMock.run.findUnique.mockResolvedValue({ id: "corrida-padrao" });
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save(payloadValido);

    expect(prismaMock.telemetry.create).toHaveBeenCalledWith({
      data: expect.objectContaining({
        runId: "corrida-padrao",
        tempoCorridaMs: 15400,
        posicaoX: 3,
        posicaoY: 4,
        direcaoAtual: "NORTE",
        estadoRobo: "EXPLORANDO",
        bateriaPct: 82,
        distFrenteCm: 12.5,
        distEsquerdaCm: 4.1,
        distDireitaCm: 15.0,
      }),
    });
  });

  // ----------------------------------------------------------
  it("deve usar valores padrão quando leitura_sensores não for enviada", async () => {
    const payloadSemSensores = { ...payloadValido };
    delete (payloadSemSensores as any).leitura_sensores;

    prismaMock.run.findUnique.mockResolvedValue({ id: "corrida-padrao" });
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save(payloadSemSensores);

    expect(prismaMock.telemetry.create).toHaveBeenCalledWith({
      data: expect.objectContaining({
        distFrenteCm: 0,
        distEsquerdaCm: 0,
        distDireitaCm: 0,
      }),
    });
  });

  // ----------------------------------------------------------
  it("deve arredondar tempo_corrida_ms, posicao_x e posicao_y para inteiro", async () => {
    const payloadComFloat = {
      ...payloadValido,
      tempo_corrida_ms: 15400.9,
      posicao_x: 3.7,
      posicao_y: 4.2,
    };
    prismaMock.run.findUnique.mockResolvedValue({ id: "corrida-padrao" });
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save(payloadComFloat);

    expect(prismaMock.telemetry.create).toHaveBeenCalledWith({
      data: expect.objectContaining({
        tempoCorridaMs: 15400,
        posicaoX: 3,
        posicaoY: 4,
      }),
    });
  });

  // ----------------------------------------------------------
  it("deve propagar erro do Prisma quando o banco falha no create", async () => {
    prismaMock.run.findUnique.mockResolvedValue({ id: "corrida-padrao" });
    prismaMock.telemetry.create.mockRejectedValue(new Error("DB offline"));

    await expect(TelemetryService.save(payloadValido)).rejects.toThrow("DB offline");
  });
});

// ============================================================
describe("TelemetryService.getLatest()", () => {
  // ----------------------------------------------------------
  it("deve retornar o registro de telemetria mais recente", async () => {
    prismaMock.telemetry.findFirst.mockResolvedValue(registroCriado);

    const resultado = await TelemetryService.getLatest();

    expect(prismaMock.telemetry.findFirst).toHaveBeenCalledWith({
      orderBy: { timestamp: "desc" },
    });
    expect(resultado).toEqual(registroCriado);
  });

  // ----------------------------------------------------------
  it("deve retornar null quando não existe nenhuma telemetria no banco", async () => {
    prismaMock.telemetry.findFirst.mockResolvedValue(null);

    const resultado = await TelemetryService.getLatest();

    expect(resultado).toBeNull();
  });

  // ----------------------------------------------------------
  it("deve propagar erro do Prisma quando o banco falha na consulta", async () => {
    prismaMock.telemetry.findFirst.mockRejectedValue(new Error("Timeout"));

    await expect(TelemetryService.getLatest()).rejects.toThrow("Timeout");
  });
});
