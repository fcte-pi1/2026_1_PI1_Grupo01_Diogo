"use client";

import { createContext, useContext, useState, ReactNode } from "react";

type CorridaContextType = {
  corridaEmAndamento: boolean;
  setCorridaEmAndamento: (v: boolean) => void;
};

const CorridaContext = createContext<CorridaContextType | undefined>(undefined);

export function CorridaProvider({ children }: { children: ReactNode }) {
  const [corridaEmAndamento, setCorridaEmAndamento] = useState(false);

  return (
    <CorridaContext.Provider
      value={{ corridaEmAndamento, setCorridaEmAndamento }}
    >
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
