import express from "express";
import cors from "cors";
import { createServer } from "http";
import { WebSocket, WebSocketServer } from "ws";
import { telemetryRoutes } from "./routes/telemetry.routes";
import { TelemetryService } from "./services/telemetry.service";
import { parseMensagem, envelope, parsePapel } from "./ws/protocol";

const app = express();

app.use(cors(), express.json());
app.use("/api/telemetria", telemetryRoutes);

const server = createServer(app);
const wss = new WebSocketServer({ server, path: "/ws" });

// Conexões dos apps web (frontend). São elas que RECEBEM a telemetria ao vivo.
// O robô (role=robo) só envia; não recebe de volta o próprio eco.
const apps = new Set<WebSocket>();

// Retransmite uma mensagem apenas para os apps web conectados.
function broadcastApps(data: string) {
  apps.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
}

wss.on("connection", (socket, req) => {
  const papel = parsePapel(req.url);

  // Só os apps entram na lista de destinatários de broadcast.
  if (papel === "app") {
    apps.add(socket);
    socket.on("close", () => apps.delete(socket));
  }

  socket.on("message", async (message) => {
    try {
      // Aceita envelope { type, payload } ou objeto cru (tratado como telemetria).
      const { type, payload } = parseMensagem(message.toString());

      switch (type) {
        case "telemetria": {
          // Persiste e retransmite a telemetria salva (enveloped) só aos apps.
          const result = await TelemetryService.save(payload);
          broadcastApps(envelope("telemetria", result));
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