import express from "express";
import cors from "cors";
import { createServer } from "http";
import { WebSocket, WebSocketServer } from "ws";
import { telemetryRoutes } from "./routes/telemetry.routes";
import { TelemetryService } from "./services/telemetry.service";
import { parseMensagem, envelope } from "./ws/protocol";

const app = express();

app.use(cors(), express.json());
app.use("/api/telemetria", telemetryRoutes);

const server = createServer(app);
const wss = new WebSocketServer({ server, path: "/ws" });

// Envia uma mensagem a todos os clientes conectados.
function broadcast(data: string) {
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
}

wss.on("connection", (socket) => {
  socket.on("message", async (message) => {
    try {
      // Aceita envelope { type, payload } ou objeto cru (tratado como telemetria).
      const { type, payload } = parseMensagem(message.toString());

      switch (type) {
        case "telemetria": {
          // Persiste e retransmite a telemetria salva (enveloped) aos clientes.
          const result = await TelemetryService.save(payload);
          broadcast(envelope("telemetria", result));
          break;
        }

        case "ping": {
          // Heartbeat: responde só ao remetente.
          socket.send(envelope("pong"));
          break;
        }

        default:
          console.warn("Tipo de mensagem WS desconhecido:", type);
      }
    } catch (error) {
      console.error("Erro ao processar mensagem WebSocket:", error);
    }
  });
});

server.listen(3000, () => console.log("Servidor online: http://localhost:3000"));