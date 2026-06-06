// mocks/mock-robo.js
const ENDPOINT = "http://localhost:3000/api/telemetria";

let tempo = 0;
let posX = 0;
let posY = 0;
let bateria = 100;

console.log("Iniciando simulacao do robo conforme o contrato (Porta 3000)...");
const meuRunIdUnico = "mock-" + Date.now();

setInterval(async () => {
  
  const payload = {
    runId: meuRunIdUnico,
    tempo_corrida_ms: tempo,
    posicao_x: posX,
    posicao_y: posY,
    direcao_atual: "NORTE",
    estado_robo: "EXPLORANDO",
    bateria_pct: bateria,
    leitura_sensores: {
      dist_frente_cm: 12.5,
      dist_esquerda_cm: 4.1,
      dist_direita_cm: 15.0
    }
  };
  
  try {
    const res = await fetch(ENDPOINT, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });

    if (res.ok) {
      console.log(`✅ Enviado -> X: ${payload.posicao_x} | Y: ${payload.posicao_y} | Bat: ${payload.bateria_pct}%`);
    } else {
      console.log(`❌ Erro: Servidor recusou os dados (Status: ${res.status}).`);
    }
  } catch (error) {
    console.log("🔌 Erro: Falha ao conectar no servidor. Ele está rodando?");
  }

  tempo += 2000; 
  posX += 1;     
  posY += 1;
  if (bateria > 10) bateria -= 1;

}, 2000);