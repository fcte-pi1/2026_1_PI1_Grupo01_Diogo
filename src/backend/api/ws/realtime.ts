import type { IncomingMessage, Server } from "http";
import { WebSocket, WebSocketServer } from "ws";
import { TelemetryService } from "../services/telemetry.service";
import { envelope, parseMensagem, parsePapel } from "./protocol";

type WebSocketWithHeartbeat = WebSocket & {
  isAlive: boolean;
};

type RealtimeOptions = {
  heartbeatIntervalMs?: number;
};

type RealtimeHandle = {
  wss: WebSocketServer;
  stop: () => Promise<void>;
};

function sendJson(socket: WebSocket, payload: unknown) {
  if (socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify(payload));
  }
}

function sendError(socket: WebSocket, code: string, message: string) {
  sendJson(socket, {
    type: "error",
    code,
    message,
  });
}

function broadcast(clients: Set<WebSocket>, data: string) {
  clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
}

export function setupRealtime(
  wss: WebSocketServer,
  options: RealtimeOptions = {}
): RealtimeHandle {
  const heartbeatIntervalMs = options.heartbeatIntervalMs ?? 30_000;
  const appClients = new Set<WebSocket>();

  const heartbeat = setInterval(() => {
    wss.clients.forEach((client) => {
      const socket = client as WebSocketWithHeartbeat;

      if (!socket.isAlive) {
        socket.terminate();
        return;
      }

      socket.isAlive = false;
      socket.ping();
    });
  }, heartbeatIntervalMs);

  wss.on("connection", (socket: WebSocket, req: IncomingMessage) => {
    const client = socket as WebSocketWithHeartbeat;
    client.isAlive = true;

    if (parsePapel(req.url) === "app") {
      appClients.add(socket);
      socket.on("close", () => appClients.delete(socket));
    }

    socket.send(
      envelope("connection", {
        event: "connected",
        transport: "websocket",
        role: parsePapel(req.url),
      })
    );

    socket.on("pong", () => {
      client.isAlive = true;
    });

    socket.on("message", async (message) => {
      try {
        const { type, payload } = parseMensagem(message.toString());

        switch (type) {
          case "telemetria": {
            if (!payload || typeof payload !== "object" || Array.isArray(payload)) {
              sendError(
                socket,
                "INVALID_PAYLOAD",
                "Mensagem WebSocket inválida: payload deve ser um objeto."
              );
              return;
            }

            const telemetria = payload as Record<string, unknown>;
            if (
              telemetria.tempo_corrida_ms === undefined ||
              telemetria.posicao_x === undefined ||
              telemetria.posicao_y === undefined ||
              telemetria.bateria_pct === undefined
            ) {
              sendError(
                socket,
                "INVALID_PAYLOAD",
                "Mensagem WebSocket inválida: campos obrigatórios ausentes."
              );
              return;
            }

            const result = await TelemetryService.save(payload);
            broadcast(appClients, envelope("telemetria", result));
            break;
          }

          case "ping": {
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

  const stop = () =>
    new Promise<void>((resolve) => {
      clearInterval(heartbeat);
      wss.clients.forEach((client) => client.terminate());
      wss.close(() => resolve());
    });

  return { wss, stop };
}

export function attachRealtime(
  server: Server,
  options: RealtimeOptions = {}
): RealtimeHandle {
  const wss = new WebSocketServer({
    server,
    path: "/ws",
  });

  return setupRealtime(wss, options);
}
