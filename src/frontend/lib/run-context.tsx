"use client";

import {
  createContext,
  useContext,
  useState,
  useEffect,
  ReactNode,
} from "react";

const API_BASE = "http://localhost:3000/api/telemetria";

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

  // O robô é quem salva a corrida (POSTa telemetria continuamente). A web
  // apenas OBSERVA: ao acompanhar, descobre qual corrida está recebendo dados
  // e desenha a trajetória completa dela.
  useEffect(() => {
    if (!corridaEmAndamento) {
      // Para o polling, mas mantém o último caminho na tela para revisão.
      return;
    }

    // Começa a observação a partir de um estado limpo.
    setTelemetria(null);
    setTelemetries([]);
    setRunIdAtual(null);

    // Corrida na qual travamos a observação. Começa nula: só passa a desenhar
    // quando o robô abre uma corrida ATIVA (EM_ANDAMENTO). Isso evita exibir a
    // última corrida já finalizada ao clicar em "Iniciar Gravação".
    let runAtivoId: string | null = null;

    const intervalo = setInterval(async () => {
      try {
        // Enquanto não travou numa corrida, procura a corrida ativa atual.
        if (!runAtivoId) {
          const resRuns = await fetch(`${API_BASE}/runs`);
          if (!resRuns.ok) return;

          const runs: { id: string; status: string }[] =
            await resRuns.json();

          const ativa = Array.isArray(runs)
            ? runs.find((r) => r.status === "EM_ANDAMENTO")
            : undefined;

          if (!ativa) return; // nenhuma corrida em andamento ainda

          runAtivoId = ativa.id;
          setRunIdAtual(ativa.id);
        }

        // Busca a trajetória completa da corrida travada (ordenada, sem perdas).
        // Continua puxando mesmo após o robô finalizá-la, para capturar o
        // último ponto (ex.: OBJETIVO_ENCONTRADO).
        const resTels = await fetch(
          `${API_BASE}/runs/${runAtivoId}/telemetries`
        );
        if (!resTels.ok) return;

        const dados: Telemetria[] = await resTels.json();
        if (!Array.isArray(dados) || dados.length === 0) return;

        setTelemetries(dados);
        setTelemetria(dados[dados.length - 1]);
      } catch (error) {
        console.error("Erro ao buscar telemetria:", error);
      }
    }, 500);

    return () => clearInterval(intervalo);
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
