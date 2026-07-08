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

// Corrida ativa (EM_ANDAMENTO) usada como destino padrão da telemetria.
const corridaAtiva = {
  id: "corrida-ativa-1",
  status: "EM_ANDAMENTO",
  startedAt: new Date("2026-06-07T09:59:00.000Z"),
  endedAt: null,
};

// Registro que o Prisma devolveria após um create bem-sucedido
const registroCriado = {
  id: "uuid-telemetria-1",
  runId: "corrida-ativa-1",
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
  it("deve anexar a telemetria à corrida ativa (EM_ANDAMENTO) quando nenhum runId é enviado", async () => {
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    const resultado = await TelemetryService.save(payloadValido);

    expect(prismaMock.run.findFirst).toHaveBeenCalledWith({
      where: { status: "EM_ANDAMENTO" },
      orderBy: { startedAt: "desc" },
    });
    // Não deve criar corrida nova quando já existe uma ativa
    expect(prismaMock.run.create).not.toHaveBeenCalled();
    expect(resultado).toEqual(registroCriado);
  });

  // ----------------------------------------------------------
  it("deve criar uma nova corrida quando não há corrida ativa e nenhum runId é enviado", async () => {
    prismaMock.run.findFirst.mockResolvedValue(null);
    prismaMock.run.create.mockResolvedValue({ id: "corrida-auto-1" });
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save(payloadValido);

    expect(prismaMock.run.create).toHaveBeenCalledWith({ data: {} });
  });

  // ----------------------------------------------------------
  it("deve usar o runId customizado enviado pelo robô e criá-lo se ainda não existir", async () => {
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
    expect(prismaMock.run.create).toHaveBeenCalledWith({
      data: { id: "corrida-123" },
    });
    // Com runId explícito não deve consultar a corrida ativa
    expect(prismaMock.run.findFirst).not.toHaveBeenCalled();
    expect(resultado.runId).toBe("corrida-123");
  });

  // ----------------------------------------------------------
  it("não deve criar corrida duplicada quando o runId enviado já existe", async () => {
    const payloadComRunId = { ...payloadValido, runId: "corrida-123" };
    prismaMock.run.findUnique.mockResolvedValue({ id: "corrida-123" });
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save(payloadComRunId);

    expect(prismaMock.run.create).not.toHaveBeenCalled();
  });

  // ----------------------------------------------------------
  it("deve mapear corretamente todos os campos do payload para o banco", async () => {
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save(payloadValido);

    expect(prismaMock.telemetry.create).toHaveBeenCalledWith({
      data: expect.objectContaining({
        runId: "corrida-ativa-1",
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
  it("deve aceitar sensores no nível raiz do payload (compatível com o mock do robô)", async () => {
    const payloadSensoresFlat = {
      tempo_corrida_ms: 1000,
      posicao_x: 1,
      posicao_y: 1,
      direcao_atual: "LESTE",
      estado_robo: "EXPLORANDO",
      bateria_pct: 90,
      dist_frente_cm: 7.7,
      dist_esquerda_cm: 8.8,
      dist_direita_cm: 9.9,
    };
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save(payloadSensoresFlat);

    expect(prismaMock.telemetry.create).toHaveBeenCalledWith({
      data: expect.objectContaining({
        distFrenteCm: 7.7,
        distEsquerdaCm: 8.8,
        distDireitaCm: 9.9,
      }),
    });
  });

  // ----------------------------------------------------------
  it("deve usar 0 como padrão quando nenhum sensor é enviado", async () => {
    const payloadSemSensores = {
      tempo_corrida_ms: 1000,
      posicao_x: 1,
      posicao_y: 1,
      bateria_pct: 90,
    };
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
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
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
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
  it("deve finalizar a corrida como CONCLUIDA quando o robô envia OBJETIVO_ENCONTRADO", async () => {
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save({
      ...payloadValido,
      estado_robo: "OBJETIVO_ENCONTRADO",
    });

    expect(prismaMock.run.updateMany).toHaveBeenCalledWith({
      where: { id: "corrida-ativa-1", status: "EM_ANDAMENTO" },
      data: expect.objectContaining({
        status: "CONCLUIDA",
        endedAt: expect.any(Date),
      }),
    });
  });

  // ----------------------------------------------------------
  it("deve finalizar a corrida como NAO_CONCLUIDA quando o robô envia ERRO", async () => {
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save({ ...payloadValido, estado_robo: "ERRO" });

    expect(prismaMock.run.updateMany).toHaveBeenCalledWith({
      where: { id: "corrida-ativa-1", status: "EM_ANDAMENTO" },
      data: expect.objectContaining({ status: "NAO_CONCLUIDA" }),
    });
  });

  // ----------------------------------------------------------
  it("não deve finalizar a corrida em estado não terminal (EXPLORANDO)", async () => {
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save({ ...payloadValido, estado_robo: "EXPLORANDO" });

    expect(prismaMock.run.updateMany).not.toHaveBeenCalled();
  });

  // ----------------------------------------------------------
  it("deve propagar erro do Prisma quando o banco falha no create", async () => {
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
    prismaMock.telemetry.create.mockRejectedValue(new Error("DB offline"));

    await expect(TelemetryService.save(payloadValido)).rejects.toThrow(
      "DB offline"
    );
  });
});

// ============================================================
// Encerramento manual da corrida (G1) + agregados (G6)
// ============================================================
describe("TelemetryService.finalizeRun()", () => {
  const telemetriasDaCorrida = [
    { id: "t1", runId: "corrida-ativa-1", tempoCorridaMs: 100, posicaoX: 0, posicaoY: 0, bateriaPct: 95 },
    { id: "t2", runId: "corrida-ativa-1", tempoCorridaMs: 5000, posicaoX: 2, posicaoY: 1, bateriaPct: 88 },
  ];

  it("deve finalizar uma corrida EM_ANDAMENTO com o status informado e os agregados calculados", async () => {
    prismaMock.telemetry.findMany.mockResolvedValue(telemetriasDaCorrida);
    prismaMock.run.updateMany.mockResolvedValue({ count: 1 });

    await TelemetryService.finalizeRun("corrida-ativa-1", "NAO_CONCLUIDA");

    expect(prismaMock.telemetry.findMany).toHaveBeenCalledWith({
      where: { runId: "corrida-ativa-1" },
      orderBy: { tempoCorridaMs: "asc" },
    });
    expect(prismaMock.run.updateMany).toHaveBeenCalledWith({
      where: { id: "corrida-ativa-1", status: "EM_ANDAMENTO" },
      data: expect.objectContaining({
        status: "NAO_CONCLUIDA",
        endedAt: expect.any(Date),
        tempoConclusaoMs: 5000,
        consumoBateriaPct: 7,
        desafioCumprido: false,
        trajetoCoordenadas: JSON.stringify([
          { x: 0, y: 0 },
          { x: 2, y: 1 },
        ]),
      }),
    });
  });

  it("deve marcar desafioCumprido=true quando o status é CONCLUIDA", async () => {
    prismaMock.telemetry.findMany.mockResolvedValue(telemetriasDaCorrida);
    prismaMock.run.updateMany.mockResolvedValue({ count: 1 });

    await TelemetryService.finalizeRun("corrida-ativa-1", "CONCLUIDA");

    expect(prismaMock.run.updateMany).toHaveBeenCalledWith({
      where: { id: "corrida-ativa-1", status: "EM_ANDAMENTO" },
      data: expect.objectContaining({ desafioCumprido: true }),
    });
  });

  it("usa NAO_CONCLUIDA como status padrão quando nenhum é informado", async () => {
    prismaMock.telemetry.findMany.mockResolvedValue([]);
    prismaMock.run.updateMany.mockResolvedValue({ count: 1 });

    await TelemetryService.finalizeRun("corrida-sem-telemetria");

    expect(prismaMock.run.updateMany).toHaveBeenCalledWith({
      where: { id: "corrida-sem-telemetria", status: "EM_ANDAMENTO" },
      data: expect.objectContaining({ status: "NAO_CONCLUIDA" }),
    });
  });

  it("não deve sobrescrever uma corrida já finalizada (guard EM_ANDAMENTO / idempotente)", async () => {
    prismaMock.telemetry.findMany.mockResolvedValue(telemetriasDaCorrida);
    // Simula o banco real: como o guard não bate (status != EM_ANDAMENTO), count vem 0.
    prismaMock.run.updateMany.mockResolvedValue({ count: 0 });

    const resultado = await TelemetryService.finalizeRun("corrida-concluida", "NAO_CONCLUIDA");

    expect(prismaMock.run.updateMany).toHaveBeenCalledWith({
      where: { id: "corrida-concluida", status: "EM_ANDAMENTO" },
      data: expect.objectContaining({ status: "NAO_CONCLUIDA" }),
    });
    expect(resultado).toEqual({ count: 0 });
  });

  it("é no-op (count 0) quando o id não existe", async () => {
    prismaMock.telemetry.findMany.mockResolvedValue([]);
    prismaMock.run.updateMany.mockResolvedValue({ count: 0 });

    const resultado = await TelemetryService.finalizeRun("id-inexistente");

    expect(resultado).toEqual({ count: 0 });
  });

  it("lida com corrida sem nenhuma telemetria (agregados nulos, sem quebrar)", async () => {
    prismaMock.telemetry.findMany.mockResolvedValue([]);
    prismaMock.run.updateMany.mockResolvedValue({ count: 1 });

    await TelemetryService.finalizeRun("corrida-vazia", "NAO_CONCLUIDA");

    expect(prismaMock.run.updateMany).toHaveBeenCalledWith({
      where: { id: "corrida-vazia", status: "EM_ANDAMENTO" },
      data: expect.objectContaining({
        tempoConclusaoMs: null,
        velocidadeMedia: null,
        consumoBateriaPct: null,
        trajetoCoordenadas: null,
      }),
    });
  });
});

// ============================================================
// G2 — nova corrida por sessão (detecção de reset do robô)
// ============================================================
describe("TelemetryService.save() — detecção de reset (G2)", () => {
  it("deve fechar a corrida órfã e abrir uma nova quando o robô reseta (tempo_corrida_ms recuado)", async () => {
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
    prismaMock.telemetry.findFirst.mockResolvedValue({
      id: "ultima-telemetria",
      runId: "corrida-ativa-1",
      tempoCorridaMs: 20000,
    });
    prismaMock.telemetry.findMany.mockResolvedValue([]);
    prismaMock.run.updateMany.mockResolvedValue({ count: 1 });
    prismaMock.run.create.mockResolvedValue({ id: "corrida-nova-1" });
    prismaMock.telemetry.create.mockResolvedValue({
      ...registroCriado,
      runId: "corrida-nova-1",
    });

    const resultado = await TelemetryService.save({
      ...payloadValido,
      tempo_corrida_ms: 500, // bem menor que os 20000 da corrida ativa
    });

    // Fecha a corrida órfã como NAO_CONCLUIDA
    expect(prismaMock.run.updateMany).toHaveBeenCalledWith({
      where: { id: "corrida-ativa-1", status: "EM_ANDAMENTO" },
      data: expect.objectContaining({ status: "NAO_CONCLUIDA" }),
    });
    // Cria uma corrida nova para a telemetria recebida
    expect(prismaMock.run.create).toHaveBeenCalledWith({ data: {} });
    expect(resultado.runId).toBe("corrida-nova-1");
  });

  it("não deve considerar reset uma pequena variação dentro da tolerância (~500ms)", async () => {
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
    prismaMock.telemetry.findFirst.mockResolvedValue({
      id: "ultima-telemetria",
      runId: "corrida-ativa-1",
      tempoCorridaMs: 15400,
    });
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save({ ...payloadValido, tempo_corrida_ms: 15300 });

    expect(prismaMock.run.create).not.toHaveBeenCalled();
    expect(prismaMock.run.updateMany).not.toHaveBeenCalled();
  });

  it("não deve considerar reset quando é a primeira telemetria da corrida ativa", async () => {
    prismaMock.run.findFirst.mockResolvedValue(corridaAtiva);
    prismaMock.telemetry.findFirst.mockResolvedValue(null); // sem telemetria anterior
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save(payloadValido);

    expect(prismaMock.run.create).not.toHaveBeenCalled();
    expect(prismaMock.run.updateMany).not.toHaveBeenCalled();
  });

  it("deve gravar tamanho_labirinto ao criar a corrida, quando enviado pelo firmware", async () => {
    prismaMock.run.findFirst.mockResolvedValue(null);
    prismaMock.run.create.mockResolvedValue({ id: "corrida-auto-1" });
    prismaMock.telemetry.create.mockResolvedValue(registroCriado);

    await TelemetryService.save({ ...payloadValido, tamanho_labirinto: 16 });

    expect(prismaMock.run.create).toHaveBeenCalledWith({
      data: { tamanhoLabirinto: 16 },
    });
  });
});

// ============================================================
describe("TelemetryService.deleteRun()", () => {
  // ----------------------------------------------------------
  it("deve deletar a corrida pelo id", async () => {
    prismaMock.run.delete.mockResolvedValue(corridaAtiva);

    await TelemetryService.deleteRun("corrida-ativa-1");

    expect(prismaMock.run.delete).toHaveBeenCalledWith({
      where: { id: "corrida-ativa-1" },
    });
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

// ============================================================
describe("TelemetryService.getRuns()", () => {
  const listaDeCorridas = [
    { id: "corrida-2", status: "EM_ANDAMENTO", startedAt: new Date("2026-06-08T09:00:00.000Z") },
    { id: "corrida-1", status: "CONCLUIDA", startedAt: new Date("2026-06-07T09:59:00.000Z") },
  ];

  // ----------------------------------------------------------
  it("deve retornar todas as corridas ordenadas da mais recente para a mais antiga", async () => {
    prismaMock.run.findMany.mockResolvedValue(listaDeCorridas);

    const resultado = await TelemetryService.getRuns();

    expect(prismaMock.run.findMany).toHaveBeenCalledWith({
      orderBy: { startedAt: "desc" },
    });
    expect(resultado).toEqual(listaDeCorridas);
  });

  // ----------------------------------------------------------
  it("deve retornar array vazio quando não há corridas cadastradas", async () => {
    prismaMock.run.findMany.mockResolvedValue([]);

    const resultado = await TelemetryService.getRuns();

    expect(resultado).toEqual([]);
  });

  // ----------------------------------------------------------
  it("deve propagar erro do Prisma quando o banco falha na consulta", async () => {
    prismaMock.run.findMany.mockRejectedValue(new Error("DB offline"));

    await expect(TelemetryService.getRuns()).rejects.toThrow("DB offline");
  });
});

// ============================================================
describe("TelemetryService.getRunById()", () => {
  // ----------------------------------------------------------
  it("deve retornar a corrida quando o id existe", async () => {
    const corrida = { id: "corrida-1", status: "CONCLUIDA" };
    prismaMock.run.findUnique.mockResolvedValue(corrida);

    const resultado = await TelemetryService.getRunById("corrida-1");

    expect(prismaMock.run.findUnique).toHaveBeenCalledWith({
      where: { id: "corrida-1" },
    });
    expect(resultado).toEqual(corrida);
  });

  // ----------------------------------------------------------
  it("deve retornar null quando o id não existe", async () => {
    prismaMock.run.findUnique.mockResolvedValue(null);

    const resultado = await TelemetryService.getRunById("inexistente");

    expect(resultado).toBeNull();
  });

  // ----------------------------------------------------------
  it("deve propagar erro do Prisma quando o banco falha na consulta", async () => {
    prismaMock.run.findUnique.mockRejectedValue(new Error("Timeout"));

    await expect(TelemetryService.getRunById("corrida-1")).rejects.toThrow("Timeout");
  });
});

// ============================================================
describe("TelemetryService.getTelemetriesByRunId()", () => {
  const telemetriasDaCorrida = [
    { id: "tel-1", runId: "corrida-1", tempoCorridaMs: 100 },
    { id: "tel-2", runId: "corrida-1", tempoCorridaMs: 200 },
  ];

  // ----------------------------------------------------------
  it("deve retornar as telemetrias da corrida ordenadas por tempo de corrida crescente", async () => {
    prismaMock.telemetry.findMany.mockResolvedValue(telemetriasDaCorrida);

    const resultado = await TelemetryService.getTelemetriesByRunId("corrida-1");

    expect(prismaMock.telemetry.findMany).toHaveBeenCalledWith({
      where: { runId: "corrida-1" },
      orderBy: { tempoCorridaMs: "asc" },
    });
    expect(resultado).toEqual(telemetriasDaCorrida);
  });

  // ----------------------------------------------------------
  it("deve retornar array vazio quando a corrida não possui telemetrias", async () => {
    prismaMock.telemetry.findMany.mockResolvedValue([]);

    const resultado = await TelemetryService.getTelemetriesByRunId("corrida-sem-dados");

    expect(resultado).toEqual([]);
  });

  // ----------------------------------------------------------
  it("deve propagar erro do Prisma quando o banco falha na consulta", async () => {
    prismaMock.telemetry.findMany.mockRejectedValue(new Error("DB offline"));

    await expect(
      TelemetryService.getTelemetriesByRunId("corrida-1")
    ).rejects.toThrow("DB offline");
  });
});
