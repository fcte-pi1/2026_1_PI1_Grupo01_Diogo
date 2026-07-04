import { WebSocket } from "ws";

const ws = new WebSocket("ws://localhost:3000/ws?role=robo");

let contador = 0;

ws.on("open", () => {
  console.log("Cliente WebSocket conectado. Enviando telemetria...");

  const intervalId = setInterval(() => {
    contador += 1;

    const payload = {
      tempo_corrida_ms: contador * 1000,
      posicao_x: contador,
      posicao_y: contador % 5,
      direcao_atual: "NORTE",
      estado_robo: contador > 5 ? "EXPLORANDO" : "INICIANDO",
      bateria_pct: 100 - contador,
      leitura_sensores: {
        dist_frente_cm: 12 + contador,
        dist_esquerda_cm: 4,
        dist_direita_cm: 7,
      },
      runId: "corrida-micromouse-v4",
    };

    ws.send(JSON.stringify({ type: "telemetria", payload }));
    console.log(`Enviado pacote ${contador}`);

    if (contador >= 8) {
      clearInterval(intervalId);
      ws.close();
      console.log("Simulação encerrada.");
    }
  }, 1000);
});

ws.on("message", (data) => {
  console.log("Mensagem recebida do servidor:", data.toString());
});

ws.on("error", (error) => {
  console.error("Erro no WebSocket:", error);
});

ws.on("close", () => {
  console.log("Conexão encerrada.");
});
