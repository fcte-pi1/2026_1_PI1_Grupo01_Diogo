import express from "express";
import cors from "cors";
import { createServer } from "http";
import { WebSocket, WebSocketServer } from "ws";
import { telemetryRoutes } from "./routes/telemetry.routes";
import { TelemetryService } from "./services/telemetry.service";

const app = express();

app.use(cors(), express.json());
app.use("/api/telemetria", telemetryRoutes);

const server = createServer(app);
const wss = new WebSocketServer({ server, path: "/ws" });

wss.on("connection", (socket) => {
  socket.on("message", async (message) => {
    try {
      const payload = JSON.parse(message.toString());
      const result = await TelemetryService.save(payload);

      wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
          client.send(JSON.stringify(result));
        }
      });
    } catch (error) {
      console.error("Erro ao processar mensagem WebSocket:", error);
    }
  });
});

server.listen(3000, () => console.log("Servidor online: http://localhost:3000"));