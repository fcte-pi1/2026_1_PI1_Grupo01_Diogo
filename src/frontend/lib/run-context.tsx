"use client";

import { createContext, useContext, useState, useEffect, useRef, ReactNode } from "react";
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
  telemetria: Telemetria | null; // Adicionamos o estado para guardar os dados
};

const CorridaContext = createContext<CorridaContextType | undefined>(undefined);

export function CorridaProvider({ children }: { children: ReactNode }) {
  const [corridaEmAndamento, setCorridaEmAndamento] = useState(false);
  const [telemetria, setTelemetria] = useState<Telemetria | null>(null);

  const runIgnorada = useRef<string | null>(null);

  useEffect(() => {
    let intervalo: NodeJS.Timeout;

    if (corridaEmAndamento) {
      // 1. Antes de ligar o cronômetro, fazemos uma busca rápida
      // para descobrir qual é a corrida velha que está no banco AGORA
      fetch("http://localhost:3000/api/telemetria/latest")
        .then((res) => res.json())
        .then((data) => {
          runIgnorada.current = data.runId; // Anota o ID fantasma
          setTelemetria(null); // Limpa a tela pro ratinho ir pro (0,0)
        })
        .catch(() => setTelemetria(null));

      console.log("Corrida iniciada! Esperando o Mock/Robô enviar dados novos...");

      // 2. Começa a perguntar ao banco a cada 500ms
      intervalo = setInterval(async () => {
        try {
          const res = await fetch("http://localhost:3000/api/telemetria/latest");
          if (res.ok) {
            const data: Telemetria = await res.json();
            
            // A NOSSA ARMADILHA:
            // Se o ID que chegou for igual ao ID que está na lista negra, ignora e sai fora!
            if (runIgnorada.current && data.runId === runIgnorada.current) {
              return; 
            }

            // Se passou da armadilha, é porque o Mock ligou e gerou um ID novo!
            setTelemetria(data);
          }
        } catch (error) {
          console.error("Erro ao buscar telemetria:", error);
        }
      }, 500);
      
    } else {
      // Se clicou em Parar, limpa a lista negra
      runIgnorada.current = null;
    }

    return () => clearInterval(intervalo);
  }, [corridaEmAndamento]);

  return (
    <CorridaContext.Provider value={{ telemetria, corridaEmAndamento, setCorridaEmAndamento }}>
      {children}
    </CorridaContext.Provider>
  );
}

// Hook customizado que encapsula o useContext
export function useCorridaContext() {
  const context = useContext(CorridaContext);
  if (!context) {
    throw new Error(
      "useCorridaContext deve ser usado dentro de CorridaProvider",
    );
  }
  return context;
}