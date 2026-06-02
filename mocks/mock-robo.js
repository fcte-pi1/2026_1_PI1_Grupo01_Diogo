// mock-robo.js
const ENDPOINT = "http://localhost:3333/api/telemetry";

let posX = 0;
let posY = 0;
let bateria = 100.0;

console.log("Iniciando simulacao do robo...");

setInterval(async () => {
  posX += 0.5;
  posY += 0.2;
  bateria -= 0.5;

  const payload = {
    runId: "corrida-simulada-001",
    batteryLevel: parseFloat(bateria.toFixed(2)),
    positionX: parseFloat(posX.toFixed(2)),
    positionY: parseFloat(posY.toFixed(2)),
    linearVelocity: 1.5
  };

  try {
    const res = await fetch(ENDPOINT, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });

    if (res.ok) {
      console.log(`Enviado -> X: ${payload.positionX} | Y: ${payload.positionY} | Bat: ${payload.batteryLevel}%`);
    } else {
      console.log("Erro: Servidor recusou os dados.");
    }
  } catch (error) {
    console.log("Erro: Falha ao conectar no servidor.");
  }
}, 2000);