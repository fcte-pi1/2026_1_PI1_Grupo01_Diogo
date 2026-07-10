export type DirecaoAtual = "NORTE" | "SUL" | "LESTE" | "OESTE";

export type EstadoRobo =
  | "EXPLORANDO"
  | "VOLTANDO"
  | "PARADO"
  | "ERRO"
  | "CONCLUIDO";

export type LeituraSensores = {
  dist_frente_cm: number;
  dist_esquerda_cm: number;
  dist_direita_cm: number;
};

export type Telemetria = {
  tempo_corrida_ms: number;
  posicao_x: number;
  posicao_y: number;
  direcao_atual: DirecaoAtual;
  estado_robo: EstadoRobo;
  bateria_pct: number;
  leitura_sensores: LeituraSensores;
};
