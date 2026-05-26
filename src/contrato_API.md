Contrato API - Telemetria Micromouse
Endpoint: POST http://[IP_LOCAL]:3000/api/telemetria
Gatilho de Envio: Disparar imediatamente a cada mudança de coordenada (X,Y). Caso permaneça na mesma célula por mais de 1000 ms, disparar um envio de segurança. Todo envio zera a contagem de tempo do gatilho.

Payload JSON e Tipagem:
O ESP32 deve enviar e a API Web deve esperar estritamente a estrutura e os tipos abaixo.

{
  "tempo_corrida_ms": 15400,    // (Int) Tempo total de prova em milissegundos
  "posicao_x": 3,               // (Int) Coordenada X na matriz (0 a 15)
  "posicao_y": 4,               // (Int) Coordenada Y na matriz (0 a 15)
  "direcao_atual": "NORTE",     // (String) Apenas: "NORTE", "SUL", "LESTE", "OESTE"
  "estado_robo": "EXPLORANDO",  // (String) Apenas: "EXPLORANDO", "VOLTANDO", "PARADO", "ERRO"
  "bateria_pct": 82,            // (Int) Nível da bateria (0 a 100)
  "leitura_sensores": {         // (Objeto) Distâncias em cm. Se livre/sem parede, enviar 999.0
    "dist_frente_cm": 12.5,     // (Float)
    "dist_esquerda_cm": 4.1,    // (Float)
    "dist_direita_cm": 15.0     // (Float)
  }
}