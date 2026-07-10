"use client";

import {
  createContext,
  useContext,
  useState,
  useEffect,
  useMemo,
  useRef,
  ReactNode,
} from "react";

import { backendHttp, backendWs } from "./backend";

// Backoff de reconexão do WebSocket: 1s, 2s, 4s... até no máximo 10s.
const RECONEXAO_MAX_MS = 10000;

export type MazeSize = 4 | 8 | 16;

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

  mazeSize: MazeSize;
  setMazeSize: (v: MazeSize) => void;

  runIdAtual: string | null;
};

const CorridaContext = createContext<CorridaContextType | undefined>(undefined);

export function CorridaProvider({ children }: { children: ReactNode }) {
  const [corridaEmAndamento, setCorridaEmAndamento] = useState(false);

  const [telemetria, setTelemetria] = useState<Telemetria | null>(null);

  const [telemetries, setTelemetries] = useState<Telemetria[]>([]);

  const [mazeSize, setMazeSize] = useState<MazeSize>(16);

  const [runIdAtual, setRunIdAtual] = useState<string | null>(null);

  const mazeSizeRef = useRef<MazeSize>(mazeSize);
  useEffect(() => {
    mazeSizeRef.current = mazeSize;
  }, [mazeSize]);

  // Ref com o runId mais recente, para ser lido pelo efeito de finalização
  // sem precisar entrar nas dependências dele (isso evitaria reconectar o
  // WebSocket toda vez que o runId mudasse durante uma observação ativa).
  const runIdAtualRef = useRef<string | null>(null);
  useEffect(() => {
    runIdAtualRef.current = runIdAtual;
  }, [runIdAtual]);

  // Marca se já estávamos observando uma corrida, para só finalizar no
  // backend quando o usuário efetivamente clicar "Parar" (transição
  // true → false), não na montagem inicial do componente.
  const observandoRef = useRef(false);

  const wsRef = useRef<WebSocket | null>(null);

  const enviarMazeSize = () => {
    const ws = wsRef.current;
    if (!ws || ws.readyState !== WebSocket.OPEN) return;

    ws.send(
      JSON.stringify({
        type: "maze_size",
        payload: { tamanho_labirinto: mazeSizeRef.current },
      }),
    );
  };

  // G1: ao parar de observar uma corrida, finaliza-a no backend (marca
  // NAO_CONCLUIDA se ainda estiver EM_ANDAMENTO). Isso fecha o ciclo de
  // salvamento quando o usuário interrompe a corrida pelo botão "Parar" —
  // sem isso, a corrida ficava presa em EM_ANDAMENTO para sempre.
  useEffect(() => {
    if (corridaEmAndamento) {
      observandoRef.current = true;
      return;
    }

    if (!observandoRef.current) return; // nunca esteve observando: nada a finalizar
    observandoRef.current = false;

    const runId = runIdAtualRef.current;
    if (!runId) return;

    fetch(`${backendHttp()}/api/telemetria/runs/${runId}/finalizar`, {
      method: "PATCH",
    }).catch(() => {
      // Falha ao finalizar (rede/backend fora do ar): a tela já parou de
      // observar e mantém o último caminho; o backend ainda finaliza
      // automaticamente se receber um estado terminal do robô depois.
    });
  }, [corridaEmAndamento]);

  useEffect(() => {
    if (!corridaEmAndamento) return;
    enviarMazeSize();
  }, [mazeSize, corridaEmAndamento]);

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

    // IDs já vistos nesta observação. Usar um Set torna a checagem de
    // duplicata O(1) em vez de percorrer o array inteiro (prev.some(...))
    // a cada pacote novo — importante em corridas longas, com muitos pontos.
    const idsVistos = new Set<string>();

    let ws: WebSocket | null = null;
    let encerrado = false; // marca o teardown para não reconectar
    let tentativa = 0; // contador de backoff
    let runCarregado: string | null = null; // corrida já "backfillada"
    let timerReconexao: ReturnType<typeof setTimeout> | undefined;

    const anexarPonto = (t: Telemetria) => {
      if (idsVistos.has(t.id)) return;
      idsVistos.add(t.id);
      setTelemetries((prev) => [...prev, t]);
      setTelemetria(t);
    };

    const tratarTelemetria = async (t: Telemetria) => {
      // Corrida nova: carrega a trajetória já salva (backfill) e depois segue
      // anexando os pontos ao vivo. Evita caminho incompleto ao conectar no meio.
      if (t.runId !== runCarregado) {
        runCarregado = t.runId;
        setRunIdAtual(t.runId);
        idsVistos.clear();

        try {
          const res = await fetch(
            `${backendHttp()}/api/telemetria/runs/${t.runId}/telemetries`,
          );
          if (res.ok) {
            const historico: Telemetria[] = await res.json();
            if (Array.isArray(historico) && historico.length > 0) {
              historico.forEach((p) => idsVistos.add(p.id));
              setTelemetries(historico);
              setTelemetria(historico[historico.length - 1]);
              return;
            }
          }
        } catch {
          // Sem backfill: segue apenas com o ponto recebido ao vivo.
        }

        idsVistos.add(t.id);
        setTelemetries([t]);
        setTelemetria(t);
        return;
      }

      anexarPonto(t);
    };

    const conectar = () => {
      ws = new WebSocket(backendWs());
      wsRef.current = ws;

      ws.onopen = () => {
        tentativa = 0;
        enviarMazeSize();
      };

      ws.onmessage = (evento) => {
        let msg: { type?: string; payload?: Telemetria } & Partial<Telemetria>;
        try {
          msg = JSON.parse(evento.data);
        } catch {
          return; // mensagem malformada — ignora
        }
        if (!msg) return;

        // Mensagens vêm no envelope { type, payload }. Só tratamos telemetria;
        // ignoramos outros tipos (ex.: pong). Tolerante a objeto cru (sem type).
        if (msg.type && msg.type !== "telemetria") return;
        const dado = (msg.payload ?? msg) as Telemetria;

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
      wsRef.current = null;
      ws?.close();
    };
  }, [corridaEmAndamento]);

  // Só cria um objeto `value` novo quando algum dado relevante realmente
  // mudou. Sem isso, todo re-render do Provider recriava o objeto e forçava
  // TODOS os componentes que usam useCorridaContext() a re-renderizar junto
  // — mesmo os que não usam o dado que mudou.
  const value = useMemo(
    () => ({
      corridaEmAndamento,
      setCorridaEmAndamento,
      telemetria,
      telemetries,
      mazeSize,
      setMazeSize,
      runIdAtual,
    }),
    [corridaEmAndamento, telemetria, telemetries, mazeSize, runIdAtual],
  );

  return (
    <CorridaContext.Provider value={value}>{children}</CorridaContext.Provider>
  );
}

export function useCorridaContext() {
  const context = useContext(CorridaContext);

  if (!context) {
    throw new Error(
      "useCorridaContext deve ser usado dentro de CorridaProvider",
    );
  }

  return context;
}
