// ============================================================
// Testes Unitários — Protocolo do WebSocket (envelope { type, payload })
// ============================================================

import { parseMensagem, envelope, parsePapel } from "../../ws/protocol";

describe("envelope()", () => {
  it("serializa no formato { type, payload }", () => {
    const s = envelope("telemetria", { posicao_x: 1 });
    expect(JSON.parse(s)).toEqual({
      type: "telemetria",
      payload: { posicao_x: 1 },
    });
  });

  it("permite payload ausente (ex.: pong)", () => {
    expect(JSON.parse(envelope("pong"))).toEqual({ type: "pong" });
  });
});

describe("parseMensagem()", () => {
  it("lê uma mensagem já no envelope { type, payload }", () => {
    const raw = JSON.stringify({
      type: "telemetria",
      payload: { posicao_x: 3 },
    });

    expect(parseMensagem(raw)).toEqual({
      type: "telemetria",
      payload: { posicao_x: 3 },
    });
  });

  it("lê um comando com seu payload", () => {
    const raw = JSON.stringify({ type: "comando", payload: { acao: "parar" } });
    expect(parseMensagem(raw)).toEqual({
      type: "comando",
      payload: { acao: "parar" },
    });
  });

  it("trata objeto CRU (sem type) como telemetria — compatibilidade retroativa", () => {
    const cru = { posicao_x: 5, runId: "r1" };
    const raw = JSON.stringify(cru);

    expect(parseMensagem(raw)).toEqual({
      type: "telemetria",
      payload: cru,
    });
  });

  it("lança quando o JSON é inválido", () => {
    expect(() => parseMensagem("{ nao json")).toThrow();
  });
});

describe("parsePapel()", () => {
  it("identifica o robô por ?role=robo", () => {
    expect(parsePapel("/ws?role=robo")).toBe("robo");
  });

  it("usa 'app' como padrão quando não há role", () => {
    expect(parsePapel("/ws")).toBe("app");
  });

  it("trata role=app explicitamente como app", () => {
    expect(parsePapel("/ws?role=app")).toBe("app");
  });

  it("usa 'app' para url ausente ou inesperada", () => {
    expect(parsePapel(undefined)).toBe("app");
    expect(parsePapel("/ws?role=qualquer")).toBe("app");
  });
});
