import { describe, it, expect, vi, beforeEach } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { Navbar } from "./navbar";

// ============================================================
// Mocks do next/navigation
// ============================================================
const mockPush = vi.fn();
let mockPathname = "/";

vi.mock("next/navigation", () => ({
  usePathname: () => mockPathname,
  useRouter: () => ({ push: mockPush }),
}));

// ============================================================
// Mock do contexto de corrida — controla corridaEmAndamento
// de forma isolada, sem precisar montar o CorridaProvider real.
// ============================================================
let mockCorridaEmAndamento = false;

vi.mock("@/lib/run-context", () => ({
  useCorridaContext: () => ({
    corridaEmAndamento: mockCorridaEmAndamento,
  }),
}));

beforeEach(() => {
  mockPush.mockClear();
  mockPathname = "/";
  mockCorridaEmAndamento = false;
});

// ============================================================
describe("Navbar", () => {
  // ----------------------------------------------------------
  it("deve exibir o logo 'MrBombastic' em qualquer rota", () => {
    render(<Navbar />);

    expect(screen.getByText("MrBombastic")).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("na rota '/', deve exibir os botões 'Nova Corrida' e 'Atual Corrida'", () => {
    mockPathname = "/";

    render(<Navbar />);

    expect(
      screen.getByRole("button", { name: "Nova Corrida" })
    ).toBeInTheDocument();
    expect(
      screen.getByRole("button", { name: "Atual Corrida" })
    ).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("na rota '/runs', deve exibir o link 'Histórico de Corridas'", () => {
    mockPathname = "/runs";

    render(<Navbar />);

    expect(screen.getByText("Histórico de Corridas")).toBeInTheDocument();
    expect(
      screen.queryByRole("button", { name: "Nova Corrida" })
    ).not.toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("em uma sub-rota de '/runs' (ex: '/runs/abc123'), também deve exibir o link de histórico", () => {
    mockPathname = "/runs/abc123";

    render(<Navbar />);

    expect(screen.getByText("Histórico de Corridas")).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("em uma rota desconhecida, não deve exibir nenhum item de navegação", () => {
    mockPathname = "/pagina-que-nao-existe";

    render(<Navbar />);

    expect(
      screen.queryByRole("button", { name: "Nova Corrida" })
    ).not.toBeInTheDocument();
    expect(screen.queryByText("Histórico de Corridas")).not.toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("o botão 'Atual Corrida' deve estar desabilitado quando não há corrida em andamento", () => {
    mockCorridaEmAndamento = false;

    render(<Navbar />);

    expect(screen.getByRole("button", { name: "Atual Corrida" })).toBeDisabled();
  });

  // ----------------------------------------------------------
  it("o botão 'Atual Corrida' deve estar habilitado quando há corrida em andamento", () => {
    mockCorridaEmAndamento = true;

    render(<Navbar />);

    expect(screen.getByRole("button", { name: "Atual Corrida" })).toBeEnabled();
  });

  // ----------------------------------------------------------
  it("deve navegar para '/runs' ao clicar em 'Atual Corrida' habilitado", async () => {
    const user = userEvent.setup();
    mockCorridaEmAndamento = true;

    render(<Navbar />);

    await user.click(screen.getByRole("button", { name: "Atual Corrida" }));

    expect(mockPush).toHaveBeenCalledWith("/runs");
  });

  // ----------------------------------------------------------
  it("deve abrir o diálogo de confirmação ao clicar em 'Nova Corrida', sem navegar imediatamente", async () => {
    const user = userEvent.setup();

    render(<Navbar />);

    await user.click(screen.getByRole("button", { name: "Nova Corrida" }));

    expect(
      screen.getByText(/iniciar uma nova corrida\?/i)
    ).toBeInTheDocument();
    expect(mockPush).not.toHaveBeenCalled();
  });

  // ----------------------------------------------------------
  it("deve navegar para '/runs' ao confirmar a criação de uma nova corrida", async () => {
    const user = userEvent.setup();

    render(<Navbar />);

    await user.click(screen.getByRole("button", { name: "Nova Corrida" }));
    await user.click(screen.getByRole("button", { name: "Confirmar" }));

    expect(mockPush).toHaveBeenCalledWith("/runs");
  });

  // ----------------------------------------------------------
  it("não deve navegar ao cancelar a criação de uma nova corrida", async () => {
    const user = userEvent.setup();

    render(<Navbar />);

    await user.click(screen.getByRole("button", { name: "Nova Corrida" }));
    await user.click(screen.getByRole("button", { name: "Cancelar" }));

    expect(mockPush).not.toHaveBeenCalled();
  });
});
