"use client";

import {
  createContext,
  useContext,
  useState,
  useEffect,
  useRef,
  ReactNode,
} from "react";

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

  const runIgnorada =
    useRef<string | null>(null);

  useEffect(() => {
    let intervalo: NodeJS.Timeout;

    if (corridaEmAndamento) {
      fetch(
        "http://localhost:3000/api/telemetria/latest"
      )
        .then((res) => res.json())
        .then((data) => {
          runIgnorada.current =
            data.runId || null;

          setTelemetria(null);
          setTelemetries([]);
          setRunIdAtual(null);
        })
        .catch(() => {
          setTelemetria(null);
          setTelemetries([]);
          setRunIdAtual(null);
        });

      console.log(
        "Corrida iniciada! Aguardando novas telemetrias..."
      );

      intervalo = setInterval(async () => {
        try {
          const res = await fetch(
            "http://localhost:3000/api/telemetria/latest"
          );

          if (!res.ok) return;

          const data: Telemetria =
            await res.json();

          if (
            runIgnorada.current &&
            data.runId ===
              runIgnorada.current
          ) {
            return;
          }

          setTelemetria(data);

          setRunIdAtual(data.runId);

          setTelemetries((anterior) => {
            const existe =
              anterior.some(
                (t) => t.id === data.id
              );

            if (existe) {
              return anterior;
            }

            return [
              ...anterior,
              data,
            ];
          });
        } catch (error) {
          console.error(
            "Erro ao buscar telemetria:",
            error
          );
        }
      }, 500);
    } else {
      runIgnorada.current = null;

      setTelemetria(null);
      setTelemetries([]);
      setRunIdAtual(null);
    }

    return () => {
      if (intervalo) {
        clearInterval(intervalo);
      }
    };
  }, [corridaEmAndamento]);

  return (
    <CorridaContext.Provider
      value={{
        telemetria,
        telemetries,
        runIdAtual,
        corridaEmAndamento,
        setCorridaEmAndamento,
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