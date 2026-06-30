// ============================================================
// Testes Unitários — TelemetryController
// ============================================================

// IMPORTANTE: o jest.mock deve vir ANTES de qualquer import
// para garantir que o módulo é interceptado corretamente.
jest.mock("../../services/telemetry.service");

import { Request, Response } from "express";
import { TelemetryController } from "../../controllers/telemetry.controller";
import { TelemetryService } from "../../services/telemetry.service";

// Cast para ter acesso aos métodos do jest.fn()
const mockSave = TelemetryService.save as jest.Mock;
const mockGetLatest = TelemetryService.getLatest as jest.Mock;

// Helpers para criar mocks de Request e Response sem precisar do Express real
const criarReq = (body: object = {}): Partial<Request> => ({ body });

const criarRes = (): Partial<Response> => {
  const res: Partial<Response> = {};
  res.status = jest.fn().mockReturnValue(res);
  res.json = jest.fn().mockReturnValue(res);
  return res;
};

// Payload válido conforme o contrato da API
const payloadValido = {
  tempo_corrida_ms: 15400,
  posicao_x: 3,
  posicao_y: 4,
  bateria_pct: 82,
};

// Registro de retorno simulado
const registroSalvo = {
  id: "uuid-1",
  runId: "corrida-padrao",
  tempoCorridaMs: 15400,
  posicaoX: 3,
  posicaoY: 4,
  bateriaPct: 82,
};

// ============================================================
describe("TelemetryController.create()", () => {
  // ----------------------------------------------------------
  it("deve retornar 201 e o registro quando o payload é válido", async () => {
    mockSave.mockResolvedValue(registroSalvo);

    const req = criarReq(payloadValido);
    const res = criarRes();

    await TelemetryController.create(req as Request, res as Response);

    expect(res.status).toHaveBeenCalledWith(201);
    expect(res.json).toHaveBeenCalledWith(registroSalvo);
    expect(mockSave).toHaveBeenCalledWith(payloadValido);
  });

  // ----------------------------------------------------------
  it("deve retornar 400 quando tempo_corrida_ms está ausente", async () => {
    const { tempo_corrida_ms, ...semTempo } = payloadValido;
    const req = criarReq(semTempo);
    const res = criarRes();

    await TelemetryController.create(req as Request, res as Response);

    expect(res.status).toHaveBeenCalledWith(400);
    expect(res.json).toHaveBeenCalledWith(
      expect.objectContaining({ error: expect.any(String) })
    );
    expect(mockSave).not.toHaveBeenCalled();
  });

  // ----------------------------------------------------------
  it("deve retornar 400 quando posicao_x está ausente", async () => {
    const { posicao_x, ...semX } = payloadValido;
    const req = criarReq(semX);
    const res = criarRes();

    await TelemetryController.create(req as Request, res as Response);

    expect(res.status).toHaveBeenCalledWith(400);
    expect(mockSave).not.toHaveBeenCalled();
  });

  // ----------------------------------------------------------
  it("deve retornar 400 quando posicao_y está ausente", async () => {
    const { posicao_y, ...semY } = payloadValido;
    const req = criarReq(semY);
    const res = criarRes();

    await TelemetryController.create(req as Request, res as Response);

    expect(res.status).toHaveBeenCalledWith(400);
    expect(mockSave).not.toHaveBeenCalled();
  });

  // ----------------------------------------------------------
  it("deve retornar 400 quando bateria_pct está ausente", async () => {
    const { bateria_pct, ...semBateria } = payloadValido;
    const req = criarReq(semBateria);
    const res = criarRes();

    await TelemetryController.create(req as Request, res as Response);

    expect(res.status).toHaveBeenCalledWith(400);
    expect(mockSave).not.toHaveBeenCalled();
  });

  // ----------------------------------------------------------
  it("deve retornar 400 quando o body está completamente vazio", async () => {
    const req = criarReq({});
    const res = criarRes();

    await TelemetryController.create(req as Request, res as Response);

    expect(res.status).toHaveBeenCalledWith(400);
  });

  // ----------------------------------------------------------
  it("deve retornar 500 quando o TelemetryService lança um erro inesperado", async () => {
    mockSave.mockRejectedValue(new Error("Falha crítica no banco"));

    const req = criarReq(payloadValido);
    const res = criarRes();

    await TelemetryController.create(req as Request, res as Response);

    expect(res.status).toHaveBeenCalledWith(500);
    expect(res.json).toHaveBeenCalledWith(
      expect.objectContaining({ error: "Internal Server Error" })
    );
  });

  // ----------------------------------------------------------
  it("a mensagem de erro do 400 deve conter 'Bad Request'", async () => {
    const req = criarReq({});
    const res = criarRes();

    await TelemetryController.create(req as Request, res as Response);

    const chamada = (res.json as jest.Mock).mock.calls[0][0];
    expect(chamada.error).toContain("Bad Request");
  });
});

// ============================================================
describe("TelemetryController.getLatest()", () => {
  // ----------------------------------------------------------
  it("deve retornar 200 com o último registro quando existe telemetria", async () => {
    mockGetLatest.mockResolvedValue(registroSalvo);

    const req = criarReq();
    const res = criarRes();

    await TelemetryController.getLatest(req as Request, res as Response);

    expect(res.json).toHaveBeenCalledWith(registroSalvo);
  });

  // ----------------------------------------------------------
  it("deve retornar objeto vazio quando não há registros no banco", async () => {
    mockGetLatest.mockResolvedValue(null);

    const req = criarReq();
    const res = criarRes();

    await TelemetryController.getLatest(req as Request, res as Response);

    expect(res.json).toHaveBeenCalledWith({});
  });

  // ----------------------------------------------------------
  it("deve retornar 500 quando o TelemetryService lança um erro", async () => {
    mockGetLatest.mockRejectedValue(new Error("Conexão perdida"));

    const req = criarReq();
    const res = criarRes();

    await TelemetryController.getLatest(req as Request, res as Response);

    expect(res.status).toHaveBeenCalledWith(500);
    expect(res.json).toHaveBeenCalledWith(
      expect.objectContaining({ error: "Internal Server Error" })
    );
  });
});

// ============================================================
// Exclusão de corrida (usada pela tabela de histórico)
// ============================================================

const mockDeleteRun = TelemetryService.deleteRun as jest.Mock;
const mockGetRunById = TelemetryService.getRunById as jest.Mock;

// Helpers que também expõem params e o método .send (usado no 204)
const criarReqComParams = (params: object): Partial<Request> => ({
  params: params as any,
});

const criarResComSend = (): Partial<Response> => {
  const res: Partial<Response> = {};
  res.status = jest.fn().mockReturnValue(res);
  res.json = jest.fn().mockReturnValue(res);
  res.send = jest.fn().mockReturnValue(res);
  return res;
};

const corridaAtiva = {
  id: "corrida-1",
  status: "EM_ANDAMENTO",
  startedAt: "2026-06-07T09:59:00.000Z",
  endedAt: null,
};

describe("TelemetryController.deleteRun()", () => {
  it("deve retornar 204 quando a corrida existe e é deletada", async () => {
    mockGetRunById.mockResolvedValue(corridaAtiva);
    mockDeleteRun.mockResolvedValue(corridaAtiva);

    const req = criarReqComParams({ id: "corrida-1" });
    const res = criarResComSend();

    await TelemetryController.deleteRun(req as Request, res as Response);

    expect(mockDeleteRun).toHaveBeenCalledWith("corrida-1");
    expect(res.status).toHaveBeenCalledWith(204);
    expect(res.send).toHaveBeenCalled();
  });

  it("deve retornar 404 quando a corrida não existe", async () => {
    mockGetRunById.mockResolvedValue(null);

    const req = criarReqComParams({ id: "inexistente" });
    const res = criarResComSend();

    await TelemetryController.deleteRun(req as Request, res as Response);

    expect(res.status).toHaveBeenCalledWith(404);
    expect(mockDeleteRun).not.toHaveBeenCalled();
  });

  it("deve retornar 500 quando o serviço falha", async () => {
    mockGetRunById.mockResolvedValue(corridaAtiva);
    mockDeleteRun.mockRejectedValue(new Error("DB offline"));

    const req = criarReqComParams({ id: "corrida-1" });
    const res = criarResComSend();

    await TelemetryController.deleteRun(req as Request, res as Response);

    expect(res.status).toHaveBeenCalledWith(500);
  });
});
