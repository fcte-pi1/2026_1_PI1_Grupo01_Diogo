import { describe, it, expect } from "vitest";
import { render, screen } from "@testing-library/react";
import { Minimapa } from "./minimap";

// ============================================================
describe("Minimapa", () => {
  // ----------------------------------------------------------
  it("deve renderizar a quantidade correta de células para o tamanho informado", () => {
    const { container } = render(<Minimapa tamanho={4} telemetries={[]} />);

    // grid 4x4 = 16 células (divs filhas diretas do grid)
    const grid = container.firstElementChild;
    expect(grid?.children.length).toBe(16);
  });

  // ----------------------------------------------------------
  it("não deve exibir o ícone do robô quando não há telemetrias", () => {
    render(<Minimapa tamanho={4} telemetries={[]} />);

    expect(screen.queryByAltText("Micromouse")).not.toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve posicionar o ícone do robô na posição da telemetria mais recente", () => {
    const telemetries = [
      { id: "1", posicaoX: 0, posicaoY: 0, estadoRobo: "EXPLORANDO" },
      { id: "2", posicaoX: 2, posicaoY: 1, estadoRobo: "EXPLORANDO" },
    ];

    const { container } = render(
      <Minimapa tamanho={4} telemetries={telemetries} />
    );

    // A posição atual é sempre a última telemetria da lista (2,1)
    expect(screen.getByAltText("Micromouse")).toBeInTheDocument();

    const grid = container.firstElementChild!;
    // índice = y * tamanho + x = 1 * 4 + 2 = 6
    const celulaEsperada = grid.children[6];
    expect(celulaEsperada.querySelector("img")).not.toBeNull();
  });

  // ----------------------------------------------------------
  it("deve exibir a bandeira de chegada quando o estado é OBJETIVO_ENCONTRADO", () => {
    const telemetries = [
      { id: "1", posicaoX: 1, posicaoY: 1, estadoRobo: "OBJETIVO_ENCONTRADO" },
    ];

    render(<Minimapa tamanho={4} telemetries={telemetries} />);

    expect(screen.getByText("🏁")).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("não deve exibir a bandeira de chegada em estados diferentes de OBJETIVO_ENCONTRADO", () => {
    const telemetries = [
      { id: "1", posicaoX: 1, posicaoY: 1, estadoRobo: "EXPLORANDO" },
    ];

    render(<Minimapa tamanho={4} telemetries={telemetries} />);

    expect(screen.queryByText("🏁")).not.toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve funcionar corretamente com uma única telemetria (posição inicial)", () => {
    const telemetries = [
      { id: "1", posicaoX: 0, posicaoY: 0, estadoRobo: "EXPLORANDO" },
    ];

    render(<Minimapa tamanho={4} telemetries={telemetries} />);

    expect(screen.getByAltText("Micromouse")).toBeInTheDocument();
  });
});
