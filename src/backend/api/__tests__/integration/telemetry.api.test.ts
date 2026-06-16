// ============================================================
// Testes de Integração — API de Telemetria (HTTP ponta a ponta)
// Sobe o servidor Express real e faz requisições HTTP via Supertest.
// O Prisma continua mockado (não toca o banco SQLite de dev).
// ============================================================

import request from "supertest";
import express from "express";
import cors from "cors";
import { telemetryRoutes } from "../../routes/telemetry.routes";

// Mock do TelemetryService para isolar o banco durante os testes de integração
jest.mock("../../services/telemetry.service", () => ({
  TelemetryService: {
    save: jest.fn(),
    getLatest: jest.fn(),
  },
}));

import { TelemetryService } from "../../services/telemetry.service";

// Monta o app Express igual ao server.ts, mas sem chamar .listen()
// Isso permite o Supertest controlar a porta.
const app = express();
app.use(cors(), express.json());
app.use("/api/telemetria", telemetryRoutes);

// Payload completo conforme contrato
const payloadCompleto = {
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

const registroSalvo = {
  id: "uuid-integracao-1",
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
  timestamp: "2026-06-07T10:00:00.000Z",
};

// ============================================================
describe("POST /api/telemetria", () => {
  // ----------------------------------------------------------
  it("deve aceitar payload válido e retornar 201 com o registro criado", async () => {
    (TelemetryService.save as jest.Mock).mockResolvedValue(registroSalvo);

    const res = await request(app)
      .post("/api/telemetria")
      .send(payloadCompleto)
      .set("Content-Type", "application/json");

    expect(res.status).toBe(201);
    expect(res.body).toMatchObject({
      runId: "corrida-padrao",
      tempoCorridaMs: 15400,
      posicaoX: 3,
      posicaoY: 4,
      bateriaPct: 82,
    });
  });

  // ----------------------------------------------------------
  it("deve rejeitar payload sem tempo_corrida_ms com 400", async () => {
    const { tempo_corrida_ms, ...semTempo } = payloadCompleto;

    const res = await request(app)
      .post("/api/telemetria")
      .send(semTempo)
      .set("Content-Type", "application/json");

    expect(res.status).toBe(400);
    expect(res.body).toHaveProperty("error");
  });

  // ----------------------------------------------------------
  it("deve rejeitar payload sem posicao_x com 400", async () => {
    const { posicao_x, ...semX } = payloadCompleto;

    const res = await request(app)
      .post("/api/telemetria")
      .send(semX)
      .set("Content-Type", "application/json");

    expect(res.status).toBe(400);
  });

  // ----------------------------------------------------------
  it("deve rejeitar payload sem bateria_pct com 400", async () => {
    const { bateria_pct, ...semBateria } = payloadCompleto;

    const res = await request(app)
      .post("/api/telemetria")
      .send(semBateria)
      .set("Content-Type", "application/json");

    expect(res.status).toBe(400);
  });

  // ----------------------------------------------------------
  it("deve rejeitar body completamente vazio com 400", async () => {
    const res = await request(app)
      .post("/api/telemetria")
      .send({})
      .set("Content-Type", "application/json");

    expect(res.status).toBe(400);
    expect(res.body.error).toMatch(/Bad Request/i);
  });

  // ----------------------------------------------------------
  it("deve retornar 500 com mensagem de erro quando o serviço falha internamente", async () => {
    (TelemetryService.save as jest.Mock).mockRejectedValue(
      new Error("Falha inesperada no banco")
    );

    const res = await request(app)
      .post("/api/telemetria")
      .send(payloadCompleto)
      .set("Content-Type", "application/json");

    expect(res.status).toBe(500);
    expect(res.body).toHaveProperty("error");
  });

  // ----------------------------------------------------------
  it("deve aceitar payload com runId customizado enviado pelo robô", async () => {
    const payloadComRunId = { ...payloadCompleto, runId: "corrida-robô-001" };
    (TelemetryService.save as jest.Mock).mockResolvedValue({
      ...registroSalvo,
      runId: "corrida-robô-001",
    });

    const res = await request(app)
      .post("/api/telemetria")
      .send(payloadComRunId)
      .set("Content-Type", "application/json");

    expect(res.status).toBe(201);
    expect(res.body.runId).toBe("corrida-robô-001");
  });

  // ----------------------------------------------------------
  it("deve responder corretamente mesmo sem leitura_sensores no payload", async () => {
    const { leitura_sensores, ...semSensores } = payloadCompleto;
    (TelemetryService.save as jest.Mock).mockResolvedValue(registroSalvo);

    const res = await request(app)
      .post("/api/telemetria")
      .send(semSensores)
      .set("Content-Type", "application/json");

    // leitura_sensores não é campo obrigatório no contrato, deve aceitar
    expect(res.status).toBe(201);
  });
});

// ============================================================
describe("GET /api/telemetria/latest", () => {
  // ----------------------------------------------------------
  it("deve retornar 200 com o último registro de telemetria", async () => {
    (TelemetryService.getLatest as jest.Mock).mockResolvedValue(registroSalvo);

    const res = await request(app).get("/api/telemetria/latest");

    expect(res.status).toBe(200);
    expect(res.body).toMatchObject({
      runId: "corrida-padrao",
      bateriaPct: 82,
    });
  });

  // ----------------------------------------------------------
  it("deve retornar 200 com objeto vazio quando não há registros", async () => {
    (TelemetryService.getLatest as jest.Mock).mockResolvedValue(null);

    const res = await request(app).get("/api/telemetria/latest");

    expect(res.status).toBe(200);
    expect(res.body).toEqual({});
  });

  // ----------------------------------------------------------
  it("deve retornar 500 quando o serviço falha na busca", async () => {
    (TelemetryService.getLatest as jest.Mock).mockRejectedValue(
      new Error("DB timeout")
    );

    const res = await request(app).get("/api/telemetria/latest");

    expect(res.status).toBe(500);
    expect(res.body).toHaveProperty("error");
  });
});
