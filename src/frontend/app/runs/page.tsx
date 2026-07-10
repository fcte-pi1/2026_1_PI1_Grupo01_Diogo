"use client";

import { TelemetriaPanel } from "@/components/runs/telemetria-panel";
import { ControlesPanel } from "@/components/runs/control-panel";
import { SelecaoLabirinto } from "@/components/runs/control-sizeMaze";
import { Minimapa } from "@/components/runs/minimap";
import { Separator } from "@/components/ui/separator";

import { useCorridaContext } from "@/lib/run-context";

export default function RunsPage() {
  const {
    telemetria,
    telemetries,
    corridaEmAndamento,
    setCorridaEmAndamento,
    mazeSize,
    setMazeSize,
  } = useCorridaContext();

  const robotAtivo =
    telemetria?.estadoRobo === "EXPLORANDO" ||
    telemetria?.estadoRobo === "VOLTANDO";

  return (
    <div className="grid grid-cols-2 gap-8">
      <div className="flex flex-col gap-4">
        <SelecaoLabirinto
          tamanho={mazeSize}
          onChange={setMazeSize}
          desabilitado={robotAtivo}
        />

        <Minimapa tamanho={mazeSize} telemetries={telemetries} />
      </div>

      <div className="flex flex-col gap-6">
        {telemetria ? (
          <TelemetriaPanel telemetria={telemetria} />
        ) : (
          <div className="border rounded-lg p-6 text-muted-foreground">
            Nenhuma telemetria recebida.
          </div>
        )}

        <Separator />

        <ControlesPanel
          corridaEmAndamento={corridaEmAndamento}
          onIniciar={() => setCorridaEmAndamento(true)}
          onParar={() => setCorridaEmAndamento(false)}
        />
      </div>
    </div>
  );
}
