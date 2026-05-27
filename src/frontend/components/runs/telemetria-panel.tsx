import { Battery, Timer, Gauge } from "lucide-react";
import { Telemetria } from "@/lib/FakeTips";
import { Separator } from "@/components/ui/separator";
import { formatarTempo } from "@/lib/utils";

export function TelemetriaPanel({ telemetria }: { telemetria: Telemetria }) {
  return (
    <div className="flex flex-col gap-6">
      <h2 className="text-2xl font-bold">Informações</h2>

      {/* Bateria e Velocidade lado a lado */}
      <div className="grid grid-cols-2 gap-4">
        <div className="flex items-center gap-3 border rounded-lg p-4">
          <Battery className="h-5 w-5" />
          <div>
            <p className="text-xs text-muted-foreground">Bateria</p>
            <p className="font-bold">{telemetria.bateria_pct}%</p>
          </div>
        </div>

        <div className="flex items-center gap-3 border rounded-lg p-4">
          <Gauge className="h-5 w-5" />
          <div>
            <p className="text-xs text-muted-foreground">Velocidade</p>
            <p className="font-bold">100m/s</p>
          </div>
        </div>
      </div>

      {/* Tempo centralizado */}
      <div className="flex items-center gap-3 border rounded-lg p-4">
        <Timer className="h-5 w-5" />
        <div>
          <p className="text-xs text-muted-foreground">Tempo</p>
          <p className="font-bold">
            {formatarTempo(telemetria.tempo_corrida_ms)}
          </p>
        </div>
      </div>

      <Separator />
    </div>
  );
}
