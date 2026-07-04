import { describe, it, expect, vi, beforeEach, afterEach } from "vitest";
import {
  render,
  screen,
  fireEvent,
  act,
  waitFor,
} from "@testing-library/react";
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

// ============================================================
// Mock de WebSocket: registra as instâncias criadas e expõe helpers
// para o teste simular open / mensagem / close.
// ============================================================
class MockWebSocket {
  static instances: MockWebSocket[] = [];
  static ultima() {
    return MockWebSocket.instances[MockWebSocket.instances.length - 1];
  }

  url: string;
  closed = false;
  onopen: (() => void) | null = null;
  onmessage: ((ev: { data: string }) => void) | null = null;
  onclose: (() => void) | null = null;
  onerror: (() => void) | null = null;

  constructor(url: string) {
    this.url = url;
    MockWebSocket.instances.push(this);
  }

  // API usada pelo run-context
  close() {
    this.closed = true;
  }
  send() {}

  // Helpers de teste
  abrir() {
    this.onopen?.();
  }
  // Telemetria no envelope { type, payload } (formato real do backend).
  receber(obj: unknown) {
    this.onmessage?.({
      data: JSON.stringify({ type: "telemetria", payload: obj }),
    });
  }
  // Mensagem arbitrária (para testar tolerância e tipos ignorados).
  receberRaw(data: string) {
    this.onmessage?.({ data });
  }
  fechar() {
    this.onclose?.();
  }
}

// Telemetrias devolvidas pelo backfill (REST /runs/:id/telemetries).
const backfillRun1 = [
  { id: "tel-1", runId: "run-1", tempoCorridaMs: 100, posicaoX: 0, posicaoY: 0, direcaoAtual: "NORTE", estadoRobo: "EXPLORANDO", bateriaPct: 90, distFrenteCm: 1, distEsquerdaCm: 1, distDireitaCm: 1, timestamp: "2026-06-08T09:00:00.000Z" },
  { id: "tel-2", runId: "run-1", tempoCorridaMs: 200, posicaoX: 1, posicaoY: 0, direcaoAtual: "NORTE", estadoRobo: "EXPLORANDO", bateriaPct: 89, distFrenteCm: 1, distEsquerdaCm: 1, distDireitaCm: 1, timestamp: "2026-06-08T09:00:01.000Z" },
];

function pontoAoVivo(id: string, runId = "run-1") {
  return { id, runId, tempoCorridaMs: 300, posicaoX: 2, posicaoY: 0, direcaoAtual: "NORTE", estadoRobo: "EXPLORANDO", bateriaPct: 88, distFrenteCm: 1, distEsquerdaCm: 1, distDireitaCm: 1, timestamp: "2026-06-08T09:00:02.000Z" };
}

// fetch usado só para o backfill.
function criarFetchBackfill(telemetries: unknown[] = backfillRun1) {
  return vi.fn().mockImplementation(() =>
    Promise.resolve({ ok: true, json: async () => telemetries })
  );
}

beforeEach(() => {
  MockWebSocket.instances = [];
  vi.stubGlobal("WebSocket", MockWebSocket);
  vi.stubGlobal("fetch", criarFetchBackfill());
});

afterEach(() => {
  vi.unstubAllGlobals();
  vi.useRealTimers();
});

function renderApp() {
  render(
    <CorridaProvider>
      <TestConsumer />
    </CorridaProvider>
  );
}

async function iniciar() {
  await act(async () => {
    fireEvent.click(screen.getByText("iniciar"));
  });
}

// ============================================================
describe("useCorridaContext()", () => {
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
describe("CorridaProvider (WebSocket)", () => {
  it("começa parado, sem corrida, e sem abrir WebSocket", () => {
    renderApp();

    expect(screen.getByTestId("em-andamento").textContent).toBe("false");
    expect(screen.getByTestId("run-id").textContent).toBe("null");
    expect(screen.getByTestId("telemetries-count").textContent).toBe("0");
    expect(MockWebSocket.instances).toHaveLength(0);
  });

  it("abre o WebSocket ao iniciar o acompanhamento", async () => {
    renderApp();
    await iniciar();

    expect(screen.getByTestId("em-andamento").textContent).toBe("true");
    expect(MockWebSocket.instances).toHaveLength(1);
    expect(MockWebSocket.ultima().url).toContain("/ws");
  });

  it("ao receber telemetria de uma corrida nova, faz backfill via REST e popula a trajetória", async () => {
    renderApp();
    await iniciar();

    const ws = MockWebSocket.ultima();
    await act(async () => {
      ws.abrir();
      ws.receber(pontoAoVivo("live-1", "run-1"));
    });

    await waitFor(() => {
      expect(screen.getByTestId("run-id").textContent).toBe("run-1");
      expect(screen.getByTestId("telemetries-count").textContent).toBe("2");
      expect(screen.getByTestId("telemetria-atual").textContent).toBe("tel-2");
    });
    expect(fetch).toHaveBeenCalledWith(
      expect.stringContaining("/runs/run-1/telemetries")
    );
  });

  it("anexa pontos novos ao vivo (mesma corrida) após o backfill", async () => {
    renderApp();
    await iniciar();
    const ws = MockWebSocket.ultima();

    await act(async () => {
      ws.receber(pontoAoVivo("live-1", "run-1")); // dispara backfill (2 pontos)
    });
    await waitFor(() =>
      expect(screen.getByTestId("telemetries-count").textContent).toBe("2")
    );

    await act(async () => {
      ws.receber(pontoAoVivo("tel-3", "run-1")); // ponto novo ao vivo
    });
    await waitFor(() => {
      expect(screen.getByTestId("telemetries-count").textContent).toBe("3");
      expect(screen.getByTestId("telemetria-atual").textContent).toBe("tel-3");
    });
  });

  it("reinicia a trajetória quando chega uma corrida diferente", async () => {
    // Backfill vazio para simplificar a asserção da corrida nova.
    vi.stubGlobal("fetch", criarFetchBackfill([]));
    renderApp();
    await iniciar();
    const ws = MockWebSocket.ultima();

    await act(async () => {
      ws.receber(pontoAoVivo("a1", "run-1"));
    });
    await waitFor(() =>
      expect(screen.getByTestId("run-id").textContent).toBe("run-1")
    );

    await act(async () => {
      ws.receber(pontoAoVivo("b1", "run-2"));
    });
    await waitFor(() => {
      expect(screen.getByTestId("run-id").textContent).toBe("run-2");
      expect(screen.getByTestId("telemetries-count").textContent).toBe("1");
    });
  });

  it("se o backfill falhar, ainda mostra o ponto recebido ao vivo", async () => {
    vi.stubGlobal("fetch", vi.fn().mockRejectedValue(new Error("rede")));
    renderApp();
    await iniciar();
    const ws = MockWebSocket.ultima();

    await act(async () => {
      ws.receber(pontoAoVivo("live-1", "run-1"));
    });
    await waitFor(() => {
      expect(screen.getByTestId("run-id").textContent).toBe("run-1");
      expect(screen.getByTestId("telemetries-count").textContent).toBe("1");
    });
  });

  it("ignora mensagens malformadas sem quebrar", async () => {
    renderApp();
    await iniciar();
    const ws = MockWebSocket.ultima();

    await act(async () => {
      ws.receberRaw("{ not json");
    });

    expect(screen.getByTestId("telemetries-count").textContent).toBe("0");
    expect(screen.getByTestId("run-id").textContent).toBe("null");
  });

  it("ignora mensagens de outros tipos do envelope (ex.: pong)", async () => {
    renderApp();
    await iniciar();
    const ws = MockWebSocket.ultima();

    await act(async () => {
      ws.receberRaw(JSON.stringify({ type: "pong" }));
    });

    expect(screen.getByTestId("telemetries-count").textContent).toBe("0");
    expect(screen.getByTestId("run-id").textContent).toBe("null");
  });

  it("aceita telemetria em objeto cru (sem envelope), por tolerância", async () => {
    vi.stubGlobal("fetch", criarFetchBackfill([])); // backfill vazio
    renderApp();
    await iniciar();
    const ws = MockWebSocket.ultima();

    await act(async () => {
      ws.receberRaw(JSON.stringify(pontoAoVivo("cru-1", "run-9")));
    });

    await waitFor(() => {
      expect(screen.getByTestId("run-id").textContent).toBe("run-9");
      expect(screen.getByTestId("telemetries-count").textContent).toBe("1");
    });
  });

  it("ao parar, mantém o último caminho e fecha o WebSocket sem reconectar", async () => {
    vi.useFakeTimers();
    renderApp();
    await act(async () => {
      fireEvent.click(screen.getByText("iniciar"));
    });
    const ws = MockWebSocket.ultima();

    await act(async () => {
      ws.receber(pontoAoVivo("live-1", "run-1"));
      await Promise.resolve();
    });

    await act(async () => {
      fireEvent.click(screen.getByText("parar"));
    });

    expect(screen.getByTestId("em-andamento").textContent).toBe("false");
    expect(ws.closed).toBe(true);
    // caminho preservado
    expect(
      Number(screen.getByTestId("telemetries-count").textContent)
    ).toBeGreaterThanOrEqual(1);

    // Mesmo avançando o tempo, não reconecta (não cria nova instância).
    const antes = MockWebSocket.instances.length;
    await act(async () => {
      await vi.advanceTimersByTimeAsync(15000);
    });
    expect(MockWebSocket.instances.length).toBe(antes);
  });

  it("reconecta com backoff quando a conexão cai durante o acompanhamento", async () => {
    vi.useFakeTimers();
    renderApp();
    await act(async () => {
      fireEvent.click(screen.getByText("iniciar"));
    });
    expect(MockWebSocket.instances).toHaveLength(1);

    // Conexão cai → agenda reconexão.
    await act(async () => {
      MockWebSocket.ultima().fechar();
    });

    // Avança o backoff (1s) → deve criar uma nova conexão.
    await act(async () => {
      await vi.advanceTimersByTimeAsync(1000);
    });

    expect(MockWebSocket.instances.length).toBe(2);
  });
});
