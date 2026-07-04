"use client";

import {
  createContext,
  useContext,
  useState,
  useEffect,
  ReactNode,
} from "react";

const API_BASE = "http://localhost:3000/api/telemetria";
const WS_URL = "ws://localhost:3000/ws";

// Backoff de reconexão do WebSocket: 1s, 2s, 4s... até no máximo 10s.
const RECONEXAO_MAX_MS = 10000;

export type Telemetria = {
  id: string;
  runId: string;

  tempoCorridaMs: number;

  posicaoX: number;
  posicaoY: number;

  direcaoAtual: string;
  estadoRobo: string;

  bateriaPct: number;

  distFrenteCm: number;
  distEsquerdaCm: number;
  distDireitaCm: number;

  timestamp: string;
};

type CorridaContextType = {
  // Indica se a tela está acompanhando (observando) a corrida ao vivo.
  corridaEmAndamento: boolean;
  setCorridaEmAndamento: (v: boolean) => void;

  telemetria: Telemetria | null;
  telemetries: Telemetria[];

  runIdAtual: string | null;
};

const CorridaContext = createContext<
  CorridaContextType | undefined
>(undefined);

export function CorridaProvider({
  children,
}: {
  children: ReactNode;
}) {
  const [corridaEmAndamento, setCorridaEmAndamento] =
    useState(false);

  const [telemetria, setTelemetria] =
    useState<Telemetria | null>(null);

  const [telemetries, setTelemetries] =
    useState<Telemetria[]>([]);

  const [runIdAtual, setRunIdAtual] =
    useState<string | null>(null);

  // O robô salva a corrida (envia telemetria pelo WebSocket). A web apenas
  // OBSERVA: ao acompanhar, assina o WebSocket e vai recebendo cada telemetria
  // nova por push (sem polling). O histórico da corrida (pontos anteriores à
  // conexão) é carregado uma vez via REST — "backfill".
  useEffect(() => {
    if (!corridaEmAndamento) {
      // Para de observar, mas mantém o último caminho na tela para revisão.
      return;
    }

    // Começa a observação a partir de um estado limpo.
    setTelemetria(null);
    setTelemetries([]);
    setRunIdAtual(null);

    let ws: WebSocket | null = null;
    let encerrado = false;              // marca o teardown para não reconectar
    let tentativa = 0;                  // contador de backoff
    let runCarregado: string | null = null; // corrida já "backfillada"
    let timerReconexao: ReturnType<typeof setTimeout> | undefined;

    const anexarPonto = (t: Telemetria) => {
      setTelemetries((prev) =>
        prev.some((p) => p.id === t.id) ? prev : [...prev, t]
      );
      setTelemetria(t);
    };

    const tratarTelemetria = async (t: Telemetria) => {
      // Corrida nova: carrega a trajetória já salva (backfill) e depois segue
      // anexando os pontos ao vivo. Evita caminho incompleto ao conectar no meio.
      if (t.runId !== runCarregado) {
        runCarregado = t.runId;
        setRunIdAtual(t.runId);

        try {
          const res = await fetch(`${API_BASE}/runs/${t.runId}/telemetries`);
          if (res.ok) {
            const historico: Telemetria[] = await res.json();
            if (Array.isArray(historico) && historico.length > 0) {
              setTelemetries(historico);
              setTelemetria(historico[historico.length - 1]);
              return;
            }
          }
        } catch {
          // Sem backfill: segue apenas com o ponto recebido ao vivo.
        }

        setTelemetries([t]);
        setTelemetria(t);
        return;
      }

      anexarPonto(t);
    };

    const conectar = () => {
      ws = new WebSocket(WS_URL);

      ws.onopen = () => {
        tentativa = 0;
      };

      ws.onmessage = (evento) => {
        let dado: Telemetria;
        try {
          dado = JSON.parse(evento.data);
        } catch {
          return; // mensagem malformada — ignora
        }
        if (!dado || !dado.runId) return;
        void tratarTelemetria(dado);
      };

      ws.onerror = () => {
        ws?.close();
      };

      ws.onclose = () => {
        if (encerrado) return;
        // Reconexão com backoff exponencial.
        const espera = Math.min(1000 * 2 ** tentativa, RECONEXAO_MAX_MS);
        tentativa += 1;
        timerReconexao = setTimeout(() => {
          if (!encerrado) conectar();
        }, espera);
      };
    };

    conectar();

    return () => {
      encerrado = true;
      if (timerReconexao) clearTimeout(timerReconexao);
      ws?.close();
    };
  }, [corridaEmAndamento]);

  return (
    <CorridaContext.Provider
      value={{
        corridaEmAndamento,
        setCorridaEmAndamento,
        telemetria,
        telemetries,
        runIdAtual,
      }}
    >
      {children}
    </CorridaContext.Provider>
  );
}

export function useCorridaContext() {
  const context =
    useContext(CorridaContext);

  if (!context) {
    throw new Error(
      "useCorridaContext deve ser usado dentro de CorridaProvider"
    );
  }

  return context;
}
