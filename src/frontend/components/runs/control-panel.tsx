"use client";

import { Button } from "@/components/ui/button";
import { Telemetria } from "@/lib/MockFakeData";
import { Play, Pause } from "lucide-react";

export function ControlesPanel({ telemetria }: { telemetria: Telemetria }) {
  const robotAtivo =
    telemetria.estado_robo === "EXPLORANDO" ||
    telemetria.estado_robo === "VOLTANDO";

  function handleIniciar() {
    console.log("Iniciar corrida");
  }

  function handleParar() {
    console.log("Parar corrida");
  }

  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-2xl font-bold">Controles</h2>

      {/* Iniciar: habilitado apenas quando robô está PARADO ou em ERRO */}
      <Button onClick={handleIniciar} disabled={robotAtivo}>
        <Play className="h-4 w-4 mr-2" />
        Iniciar
      </Button>

      {/* Parar: habilitado apenas quando robô está EXPLORANDO ou VOLTANDO */}
      <Button variant="outline" onClick={handleParar} disabled={!robotAtivo}>
        <Pause className="h-4 w-4 mr-2" />
        Parar
      </Button>
    </div>
  );
}
