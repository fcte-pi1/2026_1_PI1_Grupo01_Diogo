import { describe, it, expect, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { ControlesPanel } from "./control-panel";

// ============================================================
describe("ControlesPanel", () => {
  // ----------------------------------------------------------
  it("deve renderizar os botões 'Iniciar Gravação' e 'Parar Gravação'", () => {
    render(
      <ControlesPanel
        corridaEmAndamento={false}
        onIniciar={vi.fn()}
        onParar={vi.fn()}
      />
    );

    expect(
      screen.getByRole("button", { name: /iniciar gravação/i })
    ).toBeInTheDocument();
    expect(
      screen.getByRole("button", { name: /parar gravação/i })
    ).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("quando a corrida NÃO está em andamento: botão iniciar habilitado e parar desabilitado", () => {
    render(
      <ControlesPanel
        corridaEmAndamento={false}
        onIniciar={vi.fn()}
        onParar={vi.fn()}
      />
    );

    expect(
      screen.getByRole("button", { name: /iniciar gravação/i })
    ).toBeEnabled();
    expect(
      screen.getByRole("button", { name: /parar gravação/i })
    ).toBeDisabled();
  });

  // ----------------------------------------------------------
  it("quando a corrida ESTÁ em andamento: botão iniciar desabilitado e parar habilitado", () => {
    render(
      <ControlesPanel
        corridaEmAndamento={true}
        onIniciar={vi.fn()}
        onParar={vi.fn()}
      />
    );

    expect(
      screen.getByRole("button", { name: /iniciar gravação/i })
    ).toBeDisabled();
    expect(
      screen.getByRole("button", { name: /parar gravação/i })
    ).toBeEnabled();
  });

  // ----------------------------------------------------------
  it("deve chamar onIniciar ao clicar em 'Iniciar Gravação'", async () => {
    const user = userEvent.setup();
    const onIniciar = vi.fn();

    render(
      <ControlesPanel
        corridaEmAndamento={false}
        onIniciar={onIniciar}
        onParar={vi.fn()}
      />
    );

    await user.click(screen.getByRole("button", { name: /iniciar gravação/i }));

    expect(onIniciar).toHaveBeenCalledTimes(1);
  });

  // ----------------------------------------------------------
  it("deve chamar onParar ao clicar em 'Parar Gravação'", async () => {
    const user = userEvent.setup();
    const onParar = vi.fn();

    render(
      <ControlesPanel
        corridaEmAndamento={true}
        onIniciar={vi.fn()}
        onParar={onParar}
      />
    );

    await user.click(screen.getByRole("button", { name: /parar gravação/i }));

    expect(onParar).toHaveBeenCalledTimes(1);
  });

  // ----------------------------------------------------------
  it("não deve chamar onIniciar quando o botão está desabilitado (corrida já em andamento)", async () => {
    const user = userEvent.setup();
    const onIniciar = vi.fn();

    render(
      <ControlesPanel
        corridaEmAndamento={true}
        onIniciar={onIniciar}
        onParar={vi.fn()}
      />
    );

    await user.click(screen.getByRole("button", { name: /iniciar gravação/i }));

    expect(onIniciar).not.toHaveBeenCalled();
  });

  // ----------------------------------------------------------
  it("deve desabilitar ambos os botões quando 'carregando' é true", () => {
    render(
      <ControlesPanel
        corridaEmAndamento={false}
        carregando={true}
        onIniciar={vi.fn()}
        onParar={vi.fn()}
      />
    );

    expect(
      screen.getByRole("button", { name: /iniciar gravação/i })
    ).toBeDisabled();
    expect(
      screen.getByRole("button", { name: /parar gravação/i })
    ).toBeDisabled();
  });
});
