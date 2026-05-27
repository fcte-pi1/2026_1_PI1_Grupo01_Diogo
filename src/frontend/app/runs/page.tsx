"use client";

import { useState, useEffect } from "react";
import { Telemetria } from "@/lib/FakeTips";
import { mockTelemetria } from "@/lib/MockFakeData";
import { TelemetriaPanel } from "@/components/runs/telemetria-panel";
import { ControlesPanel } from "@/components/runs/control-panel";

export default function RunsPage() {
  const [telemetria, setTelemetria] = useState<Telemetria | null>(null);

  useEffect(() => {
    async function fetchTelemetria() {
      // Rikas - Por enquanto usamos mock — quando a API estiver pronta
      setTelemetria(mockTelemetria);
    }

    fetchTelemetria();
    const intervalo = setInterval(fetchTelemetria, 500);
    return () => clearInterval(intervalo);
  }, []);

  if (!telemetria) {
    return <p>Carregando telemetria...</p>;
  }

  return (
    <div className="grid grid-cols-2 gap-8">
      <div>
        <h2>Labirinto</h2>
        <p>Minimapa aqui</p>
      </div>
      <div>
        {/*Rikas - Por enquanto usamos fakemock para passar os dados favor mudar*/}
        <TelemetriaPanel telemetria={telemetria} />
        <ControlesPanel telemetria={telemetria} />
      </div>
    </div>
  );
}
