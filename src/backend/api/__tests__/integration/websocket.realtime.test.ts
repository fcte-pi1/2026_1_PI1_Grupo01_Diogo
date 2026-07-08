import {
  afterEach,
  beforeEach,
  describe,
  expect,
  it,
  jest,
} from "@jest/globals";
import type { IncomingMessage } from "http";
import { Duplex } from "stream";
import WebSocket, { WebSocketServer } from "ws";
import { setupRealtime } from "../../ws/realtime";
import { TelemetryService } from "../../services/telemetry.service";

jest.mock("../../services/telemetry.service", () => ({
  TelemetryService: {
    save: jest.fn(),
    getLatest: jest.fn(),
    deleteRun: jest.fn(),
    getRunById: jest.fn(),
    getRuns: jest.fn(),
    getTelemetriesByRunId: jest.fn(),
  },
}));

const mockSave = TelemetryService.save as jest.MockedFunction<
  typeof TelemetryService.save
>;

type MemoryDuplexPair = {
  client: MemoryDuplex;
  server: MemoryDuplex;
};

type ConnectedSocket = {
  client: WebSocket;
  serverSide: MemoryDuplex;
};

jest.setTimeout(10000);

class MemoryDuplex extends Duplex {
  peer?: MemoryDuplex;

  _read() {}

  _write(
    chunk: Buffer | string,
    _encoding: BufferEncoding,
    callback: (error?: Error | null) => void,
  ) {
    if (this.peer && !this.peer.destroyed) {
      this.peer.push(Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk));
    }

    callback();
  }

  _final(callback: (error?: Error | null) => void) {
    if (this.peer && !this.peer.destroyed) {
      this.peer.push(null);
    }

    callback();
  }

  _destroy(error: Error | null, callback: (error?: Error | null) => void) {
    const peer = this.peer;
    this.peer = undefined;

    if (peer && !peer.destroyed) {
      peer.peer = undefined;
      process.nextTick(() => {
        if (!peer.destroyed) {
          peer.destroy(error ?? undefined);
        }
      });
    }

    callback(error);
  }

  setTimeout(_timeout: number, callback?: () => void) {
    if (callback) {
      callback();
    }

    return this;
  }

  setNoDelay() {
    return this;
  }

  setKeepAlive() {
    return this;
  }

  ref() {
    return this;
  }

  unref() {
    return this;
  }
}

function createPair(): MemoryDuplexPair {
  const client = new MemoryDuplex();
  const server = new MemoryDuplex();
  client.peer = server;
  server.peer = client;
  return { client, server };
}

function waitForOpen(socket: WebSocket): Promise<void> {
  if (socket.readyState === WebSocket.OPEN) {
    return Promise.resolve();
  }

  return new Promise((resolve, reject) => {
    socket.once("open", () => resolve());
    socket.once("error", reject);
  });
}

function waitForClose(socket: WebSocket, timeoutMs = 2000): Promise<void> {
  return new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      reject(new Error("Timeout aguardando fechamento do WebSocket"));
    }, timeoutMs);

    socket.once("close", () => {
      clearTimeout(timeout);
      resolve();
    });

    socket.once("error", (error) => {
      clearTimeout(timeout);
      reject(error);
    });
  });
}

function waitForCondition(
  condition: () => boolean,
  timeoutMs = 2000,
  intervalMs = 10,
): Promise<void> {
  return new Promise((resolve, reject) => {
    const startedAt = Date.now();

    const tick = () => {
      if (condition()) {
        resolve();
        return;
      }

      if (Date.now() - startedAt > timeoutMs) {
        reject(new Error("Timeout aguardando condição assíncrona"));
        return;
      }

      setTimeout(tick, intervalMs);
    };

    tick();
  });
}

function parseHeaders(rawHeaders: string) {
  const headers: Record<string, string> = {};

  rawHeaders
    .split("\r\n")
    .slice(1)
    .forEach((line) => {
      const index = line.indexOf(":");
      if (index === -1) {
        return;
      }

      const name = line.slice(0, index).trim().toLowerCase();
      const value = line.slice(index + 1).trim();
      headers[name] = value;
    });

  return headers;
}

async function connectSocket(
  wss: WebSocketServer,
  role: "app" | "robo",
  options: WebSocket.ClientOptions = {},
): Promise<ConnectedSocket> {
  const pair = createPair();
  const clientUrl = `ws://localhost/ws?role=${role}`;
  let upgraded = false;
  let handshakeBuffer = Buffer.alloc(0);

  const client = new WebSocket(clientUrl, undefined, {
    createConnection: (() => pair.client) as any,
    perMessageDeflate: false,
    ...options,
  } as any);

  const openPromise = waitForOpen(client);

  pair.server.on("data", (chunk) => {
    if (upgraded) {
      return;
    }

    handshakeBuffer = Buffer.concat([handshakeBuffer, Buffer.from(chunk)]);
    const marker = handshakeBuffer.indexOf("\r\n\r\n");
    if (marker === -1) {
      return;
    }

    upgraded = true;
    const requestText = handshakeBuffer.slice(0, marker).toString("utf8");
    const headers = parseHeaders(requestText);
    const requestUrl = `/ws?role=${role}`;
    const req = {
      method: "GET",
      url: requestUrl,
      headers,
      socket: pair.server,
    } as unknown as IncomingMessage;

    const head = handshakeBuffer.slice(marker + 4);

    wss.handleUpgrade(req, pair.server, head, (ws, request) => {
      wss.emit("connection", ws, request);
    });
  });

  await openPromise;
  return { client, serverSide: pair.server };
}

beforeEach(() => {
  jest.clearAllMocks();
});

afterEach(() => {
  jest.useRealTimers();
});

describe("WebSocket realtime", () => {
  it("estabelece conexão real e faz broadcast de telemetria para apps", async () => {
    const wss = new WebSocketServer({
      noServer: true,
      perMessageDeflate: false,
    });
    const { stop } = setupRealtime(wss, { heartbeatIntervalMs: 50 });
    const mensagensApp: Array<{ type: string; payload?: unknown }> = [];

    try {
      mockSave.mockResolvedValue({
        id: "telemetria-1",
        runId: "corrida-1",
        tempoCorridaMs: 1000,
        posicaoX: 2,
        posicaoY: 3,
        direcaoAtual: "NORTE",
        estadoRobo: "EXPLORANDO",
        bateriaPct: 91,
        distFrenteCm: 12,
        distEsquerdaCm: 4,
        distDireitaCm: 7,
        timestamp: new Date("2026-06-07T10:00:00.000Z"),
      });

      const appSocket = await connectSocket(wss, "app");
      const roboSocket = await connectSocket(wss, "robo");

      appSocket.client.on("message", (data) => {
        mensagensApp.push(JSON.parse(data.toString()));
      });

      appSocket.client.send(
        JSON.stringify({
          type: "maze_size",
          payload: { tamanho_labirinto: 8 },
        }),
      );

      roboSocket.client.send(
        JSON.stringify({
          type: "telemetria",
          payload: {
            tempo_corrida_ms: 1000,
            posicao_x: 2,
            posicao_y: 3,
            direcao_atual: "NORTE",
            estado_robo: "EXPLORANDO",
            bateria_pct: 91,
            leitura_sensores: {
              dist_frente_cm: 12,
              dist_esquerda_cm: 4,
              dist_direita_cm: 7,
            },
            runId: "corrida-1",
          },
        }),
      );

      await waitForCondition(() =>
        mensagensApp.some(
          (mensagem) =>
            mensagem.type === "telemetria" &&
            (mensagem.payload as { id?: string } | undefined)?.id ===
              "telemetria-1",
        ),
      );

      expect(mockSave).toHaveBeenCalledTimes(1);
      expect(mockSave).toHaveBeenCalledWith(
        expect.objectContaining({
          tempo_corrida_ms: 1000,
          posicao_x: 2,
          posicao_y: 3,
          tamanho_labirinto: 8,
          runId: "corrida-1",
        }),
      );
      expect(
        mensagensApp.find((mensagem) => mensagem.type === "telemetria"),
      ).toEqual({
        type: "telemetria",
        payload: expect.objectContaining({
          id: "telemetria-1",
          runId: "corrida-1",
          bateriaPct: 91,
        }),
      });

      appSocket.client.close();
      roboSocket.client.close();
      await Promise.all([
        waitForClose(appSocket.client),
        waitForClose(roboSocket.client),
      ]);
    } finally {
      await stop();
    }
  });

  it("encerra a conexão quando o cliente não responde ao heartbeat", async () => {
    const wss = new WebSocketServer({
      noServer: true,
      perMessageDeflate: false,
    });
    const { stop } = setupRealtime(wss, { heartbeatIntervalMs: 50 });

    try {
      const cliente = await connectSocket(wss, "app", { autoPong: false });

      await waitForClose(cliente.client, 3000);

      expect(cliente.client.readyState).toBe(WebSocket.CLOSED);
    } finally {
      await stop();
    }
  });
});
