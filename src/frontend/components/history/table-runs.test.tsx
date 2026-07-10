import { describe, it, expect, vi, beforeEach, afterEach } from "vitest";
import { render, screen, waitFor, within } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { CorridasTable } from "./table-runs";

// ============================================================
// Mock do next/navigation — o componente usa useRouter().push()
// para navegar ao clicar em uma linha da tabela.
// ============================================================
const mockPush = vi.fn();

vi.mock("next/navigation", () => ({
  useRouter: () => ({
    push: mockPush,
  }),
}));

// ============================================================
// Dados de exemplo, alinhados com o schema do Prisma (model Run)
// ============================================================
const corridasMock = [
  {
    id: "550e8400-e29b-41d4-a716-446655440000",
    status: "CONCLUIDA",
    startedAt: "2026-06-07T09:59:00.000Z",
    endedAt: "2026-06-07T10:05:00.000Z",
  },
  {
    id: "660e8400-e29b-41d4-a716-446655440001",
    status: "EM_ANDAMENTO",
    startedAt: "2026-06-08T09:00:00.000Z",
    endedAt: null,
  },
];

// Helper para simular a resposta do fetch
function mockFetchResolvido(dados: unknown, ok = true) {
  return vi.fn().mockResolvedValue({
    ok,
    json: async () => dados,
  });
}

beforeEach(() => {
  mockPush.mockClear();
});

afterEach(() => {
  vi.unstubAllGlobals();
});

// ============================================================
describe("CorridasTable", () => {
  // ----------------------------------------------------------
  it("deve exibir o indicador de carregamento antes dos dados chegarem", () => {
    // fetch que nunca resolve, para capturar o estado de loading
    vi.stubGlobal("fetch", vi.fn().mockReturnValue(new Promise(() => {})));

    render(<CorridasTable />);

    expect(
      screen.getByText(/carregando histórico do micromouse/i)
    ).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve buscar e exibir a lista de corridas após o carregamento", async () => {
    vi.stubGlobal("fetch", mockFetchResolvido(corridasMock));

    render(<CorridasTable />);

    await waitFor(() => {
      expect(
        screen.queryByText(/carregando histórico do micromouse/i)
      ).not.toBeInTheDocument();
    });

    // Exibe os 8 primeiros caracteres do UUID de cada corrida
    expect(screen.getByText(/550e8400/)).toBeInTheDocument();
    expect(screen.getByText(/660e8400/)).toBeInTheDocument();
    expect(screen.getByText("CONCLUIDA")).toBeInTheDocument();
    expect(screen.getByText("EM_ANDAMENTO")).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve exibir mensagem de 'nenhuma corrida registrada' quando a lista vem vazia", async () => {
    vi.stubGlobal("fetch", mockFetchResolvido([]));

    render(<CorridasTable />);

    await waitFor(() => {
      expect(
        screen.getByText(/nenhuma corrida registrada no banco ainda/i)
      ).toBeInTheDocument();
    });
  });

  // ----------------------------------------------------------
  it("deve navegar para a página de detalhes ao clicar em uma linha da corrida", async () => {
    const user = userEvent.setup();
    vi.stubGlobal("fetch", mockFetchResolvido(corridasMock));

    render(<CorridasTable />);

    const linha = await screen.findByText(/550e8400/);
    await user.click(linha);

    expect(mockPush).toHaveBeenCalledWith(
      "/runs/550e8400-e29b-41d4-a716-446655440000"
    );
  });

  // ----------------------------------------------------------
  it("deve abrir o diálogo de confirmação ao clicar no ícone de apagar, sem navegar", async () => {
    const user = userEvent.setup();
    vi.stubGlobal("fetch", mockFetchResolvido(corridasMock));

    render(<CorridasTable />);

    await screen.findByText(/550e8400/);

    // O botão de apagar é o único <button> visível dentro da linha
    const botoesApagar = screen.getAllByRole("button");
    await user.click(botoesApagar[0]);

    expect(
      screen.getByText(/deseja apagar a corrida\?/i)
    ).toBeInTheDocument();
    // Clicar no ícone de apagar não deve disparar a navegação da linha
    expect(mockPush).not.toHaveBeenCalled();
  });

  // ----------------------------------------------------------
  it("deve fechar o diálogo sem apagar ao clicar em 'Não'", async () => {
    const user = userEvent.setup();
    vi.stubGlobal("fetch", mockFetchResolvido(corridasMock));

    render(<CorridasTable />);

    await screen.findByText(/550e8400/);
    const botoesApagar = screen.getAllByRole("button");
    await user.click(botoesApagar[0]);

    await user.click(screen.getByRole("button", { name: /não/i }));

    await waitFor(() => {
      expect(
        screen.queryByText(/deseja apagar a corrida\?/i)
      ).not.toBeInTheDocument();
    });
    // Continua exibindo a corrida, pois nada foi deletado
    expect(screen.getByText(/550e8400/)).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve remover a corrida da lista ao confirmar a exclusão com sucesso", async () => {
    const user = userEvent.setup();
    const fetchMock = vi.fn();
    // 1ª chamada: GET inicial da lista. 2ª chamada: DELETE da corrida.
    fetchMock
      .mockResolvedValueOnce({ ok: true, json: async () => corridasMock })
      .mockResolvedValueOnce({ ok: true, json: async () => ({}) });
    vi.stubGlobal("fetch", fetchMock);

    render(<CorridasTable />);

    await screen.findByText(/550e8400/);
    const botoesApagar = screen.getAllByRole("button");
    await user.click(botoesApagar[0]);

    await user.click(screen.getByRole("button", { name: /sim, apagar/i }));

    await waitFor(() => {
      expect(screen.queryByText(/550e8400/)).not.toBeInTheDocument();
    });
    // A corrida que não foi apagada continua na tela
    expect(screen.getByText(/660e8400/)).toBeInTheDocument();

    expect(fetchMock).toHaveBeenLastCalledWith(
      "http://localhost:3000/api/telemetria/runs/550e8400-e29b-41d4-a716-446655440000",
      { method: "DELETE" }
    );
  });

  // ----------------------------------------------------------
  it("deve manter a corrida na lista quando o servidor falha ao deletar", async () => {
    const user = userEvent.setup();
    const fetchMock = vi.fn();
    fetchMock
      .mockResolvedValueOnce({ ok: true, json: async () => corridasMock })
      .mockResolvedValueOnce({ ok: false, status: 500, json: async () => ({}) });
    vi.stubGlobal("fetch", fetchMock);

    render(<CorridasTable />);

    await screen.findByText(/550e8400/);
    const botoesApagar = screen.getAllByRole("button");
    await user.click(botoesApagar[0]);
    await user.click(screen.getByRole("button", { name: /sim, apagar/i }));

    // Aguarda o diálogo fechar (fim do fluxo de exclusão)
    await waitFor(() => {
      expect(
        screen.queryByText(/deseja apagar a corrida\?/i)
      ).not.toBeInTheDocument();
    });

    // Como o servidor falhou, a corrida continua na lista
    expect(screen.getByText(/550e8400/)).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("não deve quebrar a tela quando a busca inicial de corridas falha (erro de rede)", async () => {
    vi.stubGlobal("fetch", vi.fn().mockRejectedValue(new Error("Falha de rede")));

    render(<CorridasTable />);

    await waitFor(() => {
      expect(
        screen.queryByText(/carregando histórico do micromouse/i)
      ).not.toBeInTheDocument();
    });

    // Sem dados carregados, cai no estado de lista vazia
    expect(
      screen.getByText(/nenhuma corrida registrada no banco ainda/i)
    ).toBeInTheDocument();
  });
});
