"use client";

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

  function classificarCelula(x: number, y: number) {
    if (
      posicaoAtual &&
      posicaoAtual.posicaoX === x &&
      posicaoAtual.posicaoY === y
    ) {
      return "atual";
    }

    const visitada = telemetries.some(
      (t) =>
        t.posicaoX === x &&
        t.posicaoY === y
    );

    return visitada
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