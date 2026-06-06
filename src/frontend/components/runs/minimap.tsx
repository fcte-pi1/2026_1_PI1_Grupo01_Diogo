type Posicao = { x: number; y: number };

type MinimapProps = {
  tamanho: number;
  posicoes: Posicao[];
  posicaoAtual: Posicao;
};

export function Minimapa({ tamanho, posicoes, posicaoAtual }: MinimapProps) {
  function classificarCelula(x: number, y: number) {
    if (posicaoAtual.x === x && posicaoAtual.y === y) return "atual";
    if (posicoes.some((p) => p.x === x && p.y === y)) return "visitada";
    return "inexplorada";
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
      {Array.from({ length: tamanho * tamanho }).map((_, index) => {
        const x = index % tamanho;
        const y = Math.floor(index / tamanho);
        const tipo = classificarCelula(x, y);

        return (
          <div
            key={index}
            style={{
              backgroundColor: tipo === "visitada" ? "#888" : "black",
              border: "1px solid #222",
              position: "relative",
            }}
          >
            {tipo === "atual" && (
              <img
                src="/RatoiconMap.png"
                alt="rato"
                style={{
                  width: "100%",
                  height: "100%",
                  imageRendering: "pixelated",
                  position: "absolute",
                  top: 0,
                  left: 0,
                }}
              />
            )}
          </div>
        );
      })}
    </div>
  );
}
