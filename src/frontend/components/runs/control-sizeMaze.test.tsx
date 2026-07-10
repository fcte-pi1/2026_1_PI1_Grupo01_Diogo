import { describe, it, expect, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { SelecaoLabirinto } from "./control-sizeMaze";

// ============================================================
describe("SelecaoLabirinto", () => {
  // ----------------------------------------------------------
  it("deve renderizar as três opções de tamanho (4x4, 8x8, 16x16)", () => {
    render(<SelecaoLabirinto tamanho={8} onChange={vi.fn()} />);

    expect(screen.getByRole("tab", { name: "4x4" })).toBeInTheDocument();
    expect(screen.getByRole("tab", { name: "8x8" })).toBeInTheDocument();
    expect(screen.getByRole("tab", { name: "16x16" })).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve marcar a aba correspondente ao tamanho atual como selecionada", () => {
    render(<SelecaoLabirinto tamanho={8} onChange={vi.fn()} />);

    expect(screen.getByRole("tab", { name: "8x8" })).toHaveAttribute(
      "aria-selected",
      "true"
    );
    expect(screen.getByRole("tab", { name: "4x4" })).toHaveAttribute(
      "aria-selected",
      "false"
    );
  });

  // ----------------------------------------------------------
  it("deve chamar onChange com o valor numérico correto ao selecionar outro tamanho", async () => {
    const user = userEvent.setup();
    const onChange = vi.fn();

    render(<SelecaoLabirinto tamanho={4} onChange={onChange} />);

    await user.click(screen.getByRole("tab", { name: "16x16" }));

    expect(onChange).toHaveBeenCalledWith(16);
  });

  // ----------------------------------------------------------
  it("não deve estar desabilitado por padrão", () => {
    render(<SelecaoLabirinto tamanho={4} onChange={vi.fn()} />);

    expect(screen.getByRole("tab", { name: "4x4" })).toBeEnabled();
    expect(screen.getByRole("tab", { name: "8x8" })).toBeEnabled();
    expect(screen.getByRole("tab", { name: "16x16" })).toBeEnabled();
  });

  // ----------------------------------------------------------
  it("deve desabilitar todas as abas quando 'desabilitado' é true", () => {
    render(
      <SelecaoLabirinto tamanho={4} onChange={vi.fn()} desabilitado={true} />
    );

    expect(screen.getByRole("tab", { name: "4x4" })).toBeDisabled();
    expect(screen.getByRole("tab", { name: "8x8" })).toBeDisabled();
    expect(screen.getByRole("tab", { name: "16x16" })).toBeDisabled();
  });

  // ----------------------------------------------------------
  it("não deve chamar onChange ao clicar em uma aba desabilitada", async () => {
    const user = userEvent.setup();
    const onChange = vi.fn();

    render(
      <SelecaoLabirinto
        tamanho={4}
        onChange={onChange}
        desabilitado={true}
      />
    );

    await user.click(screen.getByRole("tab", { name: "16x16" }));

    expect(onChange).not.toHaveBeenCalled();
  });
});
