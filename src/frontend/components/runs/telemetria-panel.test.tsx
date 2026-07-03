import { describe, it, expect } from "vitest";
import { render, screen } from "@testing-library/react";
import { TelemetriaPanel, type TelemetriaUI } from "./telemetria-panel";

// ============================================================
const telemetriaBase: TelemetriaUI = {
  id: "tel-1",
  runId: "corrida-1",
  tempoCorridaMs: 90000, // 1min 30s
  posicaoX: 3,
  posicaoY: 4,
  direcaoAtual: "NORTE",
  estadoRobo: "EXPLORANDO",
  bateriaPct: 82,
  distFrenteCm: 12.53,
  distEsquerdaCm: 4.1,
  distDireitaCm: 15.0,
};

// ============================================================
describe("TelemetriaPanel", () => {
  // ----------------------------------------------------------
  it("deve exibir o estado traduzido do robô", () => {
    render(<TelemetriaPanel telemetria={telemetriaBase} />);

    // "EXPLORANDO" é traduzido para "Em andamento" pelo utils.ts
    expect(screen.getByText("Em andamento")).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve exibir a porcentagem de bateria", () => {
    render(<TelemetriaPanel telemetria={telemetriaBase} />);

    expect(screen.getByText("82%")).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve exibir o tempo de corrida formatado como HH:MM:SS", () => {
    render(<TelemetriaPanel telemetria={telemetriaBase} />);

    // 90000ms = 00:01:30
    expect(screen.getByText("00:01:30")).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve exibir a posição no formato (x,y)", () => {
    render(<TelemetriaPanel telemetria={telemetriaBase} />);

    expect(screen.getByText(/\(3,\s*4\)/)).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve exibir a direção atual", () => {
    render(<TelemetriaPanel telemetria={telemetriaBase} />);

    expect(screen.getByText("NORTE")).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve exibir as três distâncias dos sensores arredondadas para 1 casa decimal", () => {
    render(<TelemetriaPanel telemetria={telemetriaBase} />);

    // 12.53 -> 12.5 | 4.1 -> 4.1 | 15.0 -> 15.0
    expect(screen.getByText("12.5 cm")).toBeInTheDocument();
    expect(screen.getByText("4.1 cm")).toBeInTheDocument();
    expect(screen.getByText("15.0 cm")).toBeInTheDocument();
  });

  // ----------------------------------------------------------
  it("deve exibir um estado desconhecido (não mapeado) como texto cru, sem quebrar", () => {
    render(
      <TelemetriaPanel
        telemetria={{ ...telemetriaBase, estadoRobo: "ESTADO_NOVO_DA_API" }}
      />
    );

    expect(screen.getByText("ESTADO_NOVO_DA_API")).toBeInTheDocument();
  });
});
