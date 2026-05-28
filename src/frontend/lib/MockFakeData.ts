export const mockCorridas = [
  { id: 1, nome: "Corrida 1", index: 1 },
  { id: 2, nome: "Corrida 2", index: 2 },
  { id: 4, nome: "Corrida 3", index: 4 },
];

export const mockTelemetria = {
  tempo_corrida_ms: 62000,
  posicao_x: 3,
  posicao_y: 4,
  direcao_atual: "NORTE" as const,
  estado_robo: "EXPLORANDO" as const,
  bateria_pct: 82,
  leitura_sensores: {
    dist_frente_cm: 12.5,
    dist_esquerda_cm: 4.1,
    dist_direita_cm: 15.0,
  },
};
