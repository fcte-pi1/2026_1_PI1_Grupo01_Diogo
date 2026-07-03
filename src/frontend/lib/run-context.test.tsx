import { describe, it, expect, vi, beforeEach, afterEach } from "vitest";
import { render, screen, fireEvent, act } from "@testing-library/react";
import { CorridaProvider, useCorridaContext } from "./run-context";

// ============================================================
// Componente auxiliar que expõe o contexto na tela, para podermos
// inspecionar seus valores e disparar as ações via clique.
// ============================================================
function TestConsumer() {
  const ctx = useCorridaContext();
  return (
    <div>
      <div data-testid="em-andamento">{String(ctx.corridaEmAndamento)}</div>
      <div data-testid="run-id">{ctx.runIdAtual ?? "null"}</div>
      <div data-testid="telemetries-count">{ctx.telemetries.length}</div>
      <div data-testid="telemetria-atual">
        {ctx.telemetria ? ctx.telemetria.id : "null"}
      </div>
      <button onClick={() => ctx.setCorridaEmAndamento(true)}>iniciar</button>
      <button onClick={() => ctx.setCorridaEmAndamento(false)}>parar</button>
    </div>
  );
}

const telemetriasMock = [
  { id: "tel-1", runId: "run-1", tempoCorridaMs: 100, posicaoX: 0, posicaoY: 0, direcaoAtual: "NORTE", estadoRobo: "EXPLORANDO", bateriaPct: 90, distFrenteCm: 1, distEsquerdaCm: 1, distDireitaCm: 1, timestamp: "2026-06-08T09:00:00.000Z" },
  { id: "tel-2", runId: "run-1", tempoCorridaMs: 200, posicaoX: 1, posicaoY: 0, direcaoAtual: "NORTE", estadoRobo: "EXPLORANDO", bateriaPct: 89, distFrenteCm: 1, distEsquerdaCm: 1, distDireitaCm: 1, timestamp: "2026-06-08T09:00:01.000Z" },
];

// Monta um mock de fetch que responde de forma diferente conforme a rota chamada
function criarFetchMock({
  runs = [{ id: "run-1", status: "EM_ANDAMENTO" }],
  telemetries = telemetriasMock,
}: { runs?: unknown[]; telemetries?: unknown[] } = {}) {
  return vi.fn().mockImplementation((url: string) => {
    if (url.includes("/telemetries")) {
      return Promise.resolve({ ok: true, json: async () => telemetries });
    }
    if (url.endsWith("/runs")) {
      return Promise.resolve({ ok: true, json: async () => runs });
    }
    return Promise.resolve({ ok: false, json: async () => ({}) });
  });
}

beforeEach(() => {
  vi.useFakeTimers();
});

afterEach(() => {
  vi.useRealTimers();
  vi.unstubAllGlobals();
});

// ============================================================
describe("useCorridaContext()", () => {
  // ----------------------------------------------------------
  it("deve lançar erro quando usado fora do CorridaProvider", () => {
    const consoleSpy = vi.spyOn(console, "error").mockImplementation(() => {});

    function SemProvider() {
      useCorridaContext();
      return null;
    }

    expect(() => render(<SemProvider />)).toThrow(
      "useCorridaContext deve ser usado dentro de CorridaProvider"
    );

    consoleSpy.mockRestore();
  });
});

// ============================================================
describe("CorridaProvider", () => {
  // ----------------------------------------------------------
  it("deve começar com o estado inicial: parado, sem corrida e sem telemetrias", () => {
    vi.stubGlobal("fetch", criarFetchMock());

    render(
      <CorridaProvider>
        <TestConsumer />
      </CorridaProvider>
    );

    expect(screen.getByTestId("em-andamento").textContent).toBe("false");
    expect(screen.getByTestId("run-id").textContent).toBe("null");
    expect(screen.getByTestId("telemetries-count").textContent).toBe("0");
  });

  // ----------------------------------------------------------
  it("deve detectar a corrida ativa e popular as telemetrias após iniciar o acompanhamento", async () => {
    vi.stubGlobal("fetch", criarFetchMock());

    render(
      <CorridaProvider>
        <TestConsumer />
      </CorridaProvider>
    );

    fireEvent.click(screen.getByText("iniciar"));
    expect(screen.getByTestId("em-andamento").textContent).toBe("true");

    // Avança 1 ciclo de polling (1000ms) e espera as promises resolverem
    await act(async () => {
      await vi.advanceTimersByTimeAsync(1000);
    });

    expect(screen.getByTestId("run-id").textContent).toBe("run-1");
    expect(screen.getByTestId("telemetries-count").textContent).toBe("2");
    expect(screen.getByTestId("telemetria-atual").textContent).toBe("tel-2");
  });

  // ----------------------------------------------------------
  it("não deve travar em nenhuma corrida quando não há corrida EM_ANDAMENTO", async () => {
    vi.stubGlobal("fetch", criarFetchMock({ runs: [] }));

    render(
      <CorridaProvider>
        <TestConsumer />
      </CorridaProvider>
    );

    fireEvent.click(screen.getByText("iniciar"));
    await act(async () => {
      await vi.advanceTimersByTimeAsync(1000);
    });

    expect(screen.getByTestId("run-id").textContent).toBe("null");
    expect(screen.getByTestId("telemetries-count").textContent).toBe("0");
  });

  // ----------------------------------------------------------
  it("após travar numa corrida, não deve mais consultar /runs nos ciclos seguintes", async () => {
    const fetchMock = criarFetchMock();
    vi.stubGlobal("fetch", fetchMock);

    render(
      <CorridaProvider>
        <TestConsumer />
      </CorridaProvider>
    );

    fireEvent.click(screen.getByText("iniciar"));
    await act(async () => {
      await vi.advanceTimersByTimeAsync(1000);
    }); // 1º ciclo: consulta /runs e trava

    fetchMock.mockClear();
    await act(async () => {
      await vi.advanceTimersByTimeAsync(1000);
    }); // 2º ciclo: já travado

    const chamou_runs = fetchMock.mock.calls.some(([url]) =>
      String(url).endsWith("/runs")
    );
    const chamou_telemetries = fetchMock.mock.calls.some(([url]) =>
      String(url).includes("/telemetries")
    );

    expect(chamou_runs).toBe(false);
    expect(chamou_telemetries).toBe(true);
  });

  // ----------------------------------------------------------
  it("deve parar o polling ao definir corridaEmAndamento como false, mantendo o último estado na tela", async () => {
    const fetchMock = criarFetchMock();
    vi.stubGlobal("fetch", fetchMock);

    render(
      <CorridaProvider>
        <TestConsumer />
      </CorridaProvider>
    );

    fireEvent.click(screen.getByText("iniciar"));
    await act(async () => {
      await vi.advanceTimersByTimeAsync(1000);
    });
    expect(screen.getByTestId("telemetries-count").textContent).toBe("2");

    fireEvent.click(screen.getByText("parar"));
    expect(screen.getByTestId("em-andamento").textContent).toBe("false");
    // O último caminho permanece na tela para revisão
    expect(screen.getByTestId("telemetries-count").textContent).toBe("2");

    fetchMock.mockClear();
    await act(async () => {
      await vi.advanceTimersByTimeAsync(3000);
    });

    // Com o polling parado, nenhuma nova chamada deve ocorrer
    expect(fetchMock).not.toHaveBeenCalled();
  });

  // ----------------------------------------------------------
  it("não deve quebrar a aplicação quando o fetch falha (erro de rede)", async () => {
    const consoleSpy = vi.spyOn(console, "error").mockImplementation(() => {});
    vi.stubGlobal("fetch", vi.fn().mockRejectedValue(new Error("Falha de rede")));

    render(
      <CorridaProvider>
        <TestConsumer />
      </CorridaProvider>
    );

    fireEvent.click(screen.getByText("iniciar"));
    await act(async () => {
      await vi.advanceTimersByTimeAsync(1000);
    });

    // Estado permanece consistente mesmo com erro
    expect(screen.getByTestId("run-id").textContent).toBe("null");
    expect(screen.getByTestId("telemetries-count").textContent).toBe("0");

    consoleSpy.mockRestore();
  });

  // ----------------------------------------------------------
  it("deve reiniciar do zero (limpar estado anterior) ao iniciar um novo acompanhamento", async () => {
    const fetchMock = criarFetchMock();
    vi.stubGlobal("fetch", fetchMock);

    render(
      <CorridaProvider>
        <TestConsumer />
      </CorridaProvider>
    );

    // Primeira observação: popula telemetrias
    fireEvent.click(screen.getByText("iniciar"));
    await act(async () => {
      await vi.advanceTimersByTimeAsync(1000);
    });
    expect(screen.getByTestId("telemetries-count").textContent).toBe("2");

    fireEvent.click(screen.getByText("parar"));

    // Segunda observação: deve começar limpo (zerado) antes do próximo fetch resolver
    vi.stubGlobal(
      "fetch",
      vi.fn().mockReturnValue(new Promise(() => {})) // nunca resolve, para capturar o estado limpo
    );
    fireEvent.click(screen.getByText("iniciar"));

    expect(screen.getByTestId("telemetries-count").textContent).toBe("0");
    expect(screen.getByTestId("run-id").textContent).toBe("null");
  });
});
