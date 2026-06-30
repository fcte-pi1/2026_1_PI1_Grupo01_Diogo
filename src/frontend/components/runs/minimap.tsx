"use client";

import { useMemo } from "react";

type Telemetry = {
  id: string;
  posicaoX: number;
  posicaoY: number;
  estadoRobo: string;
};

type MinimapaProps = {
  tamanho: number;
  telemetries: Telemetry[];
};

export function Minimapa({
  tamanho,
  telemetries,
}: MinimapaProps) {
  const posicaoAtual =
    telemetries.length > 0
      ? telemetries[telemetries.length - 1]
      : null;

  // Pré-computa as células visitadas uma vez por atualização (Set de "x,y").
  // Antes, cada célula do grid varria toda a lista de telemetrias (.some),
  // dando O(células × telemetrias) por render. Com o Set, a checagem é O(1),
  // baixando o custo total para O(células + telemetrias).
  const visitadas = useMemo(() => {
    const conjunto = new Set<string>();
    for (const t of telemetries) {
      conjunto.add(`${t.posicaoX},${t.posicaoY}`);
    }
    return conjunto;
  }, [telemetries]);

  function classificarCelula(x: number, y: number) {
    if (
      posicaoAtual &&
      posicaoAtual.posicaoX === x &&
      posicaoAtual.posicaoY === y
    ) {
      return "atual";
    }

    return visitadas.has(`${x},${y}`)
      ? "visitada"
      : "inexplorada";
  }

  return (
    <div
      className="border"
      style={{
        display: "grid",
        gridTemplateColumns: `repeat(${tamanho}, 1fr)`,
        width: "100%",
        aspectRatio: "1 / 1",
      }}
    >
      {Array.from({
        length: tamanho * tamanho,
      }).map((_, index) => {
        const x = index % tamanho;
        const y = Math.floor(index / tamanho);

        const tipo = classificarCelula(x, y);

        return (
          <div
            key={index}
            style={{
              backgroundColor:
                tipo === "visitada"
                  ? "#888"
                  : "#000",
              border: "1px solid #222",
              position: "relative",
            }}
          >
            {tipo === "atual" && (
              <img
                src="/RatoiconMap.png"
                alt="Micromouse"
                style={{
                  width: "100%",
                  height: "100%",
                  objectFit: "contain",
                  imageRendering:
                    "pixelated",
                  position: "absolute",
                  inset: 0,
                }}
              />
            )}

            {tipo === "atual" &&
              posicaoAtual?.estadoRobo ===
                "OBJETIVO_ENCONTRADO" && (
                <div
                  className="
                    absolute
                    inset-0
                    flex
                    items-center
                    justify-center
                    text-xs
                    font-bold
                    text-green-400
                  "
                >
                  🏁
                </div>
              )}
          </div>
        );
      })}
    </div>
  );
}