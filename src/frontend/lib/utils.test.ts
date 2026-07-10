import { describe, it, expect } from "vitest";
import { cn, formatarTempo, traduzirEstado } from "./utils";

// ============================================================
describe("cn()", () => {
  // ----------------------------------------------------------
  it("deve combinar múltiplas classes em uma única string", () => {
    expect(cn("p-4", "text-red-500")).toBe("p-4 text-red-500");
  });

  // ----------------------------------------------------------
  it("deve remover classes condicionais falsy", () => {
    expect(cn("p-4", false && "hidden", undefined, "text-white")).toBe(
      "p-4 text-white"
    );
  });

  // ----------------------------------------------------------
  it("deve mesclar classes Tailwind conflitantes mantendo a última", () => {
    // twMerge deve resolver o conflito entre p-2 e p-4, mantendo p-4
    expect(cn("p-2", "p-4")).toBe("p-4");
  });
});

// ============================================================
describe("formatarTempo()", () => {
  // ----------------------------------------------------------
  it("deve formatar 0ms como 00:00:00", () => {
    expect(formatarTempo(0)).toBe("00:00:00");
  });

  // ----------------------------------------------------------
  it("deve formatar segundos corretamente (ex: 5000ms = 5s)", () => {
    expect(formatarTempo(5000)).toBe("00:00:05");
  });

  // ----------------------------------------------------------
  it("deve formatar minutos e segundos corretamente", () => {
    // 90 segundos = 1 minuto e 30 segundos
    expect(formatarTempo(90000)).toBe("00:01:30");
  });

  // ----------------------------------------------------------
  it("deve formatar horas, minutos e segundos corretamente", () => {
    // 1h 2min 3s = 3723 segundos = 3723000 ms
    expect(formatarTempo(3723000)).toBe("01:02:03");
  });

  // ----------------------------------------------------------
  it("deve truncar milissegundos parciais (arredondar para baixo)", () => {
    // 1999ms deve ser tratado como 1 segundo completo, não 2
    expect(formatarTempo(1999)).toBe("00:00:01");
  });

  // ----------------------------------------------------------
  it("deve preencher com zero à esquerda quando o valor é menor que 10", () => {
    expect(formatarTempo(9000)).toBe("00:00:09");
  });
});

// ============================================================
describe("traduzirEstado()", () => {
  // ----------------------------------------------------------
  it.each([
    ["EXPLORANDO", "Em andamento"],
    ["VOLTANDO", "Em andamento"],
    ["PARADO", "Pausado"],
    ["ERRO", "Erro"],
    ["CONCLUIDO", "Concluído"],
    ["OBJETIVO_ENCONTRADO", "Objetivo encontrado"],
  ])("deve traduzir o estado '%s' para '%s'", (estado, traducaoEsperada) => {
    expect(traduzirEstado(estado)).toBe(traducaoEsperada);
  });

  // ----------------------------------------------------------
  it("deve retornar o próprio valor quando o estado é desconhecido (fallback)", () => {
    expect(traduzirEstado("ESTADO_INEXISTENTE")).toBe("ESTADO_INEXISTENTE");
  });

  // ----------------------------------------------------------
  it("deve retornar string vazia quando o estado é uma string vazia", () => {
    expect(traduzirEstado("")).toBe("");
  });
});
