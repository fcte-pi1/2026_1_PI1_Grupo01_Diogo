"use client";

import { useState, useEffect } from "react";
import { Telemetria } from "@/lib/MockFakeData";
import { mockTelemetria } from "@/lib/MockFakeData";
import { TelemetriaPanel } from "@/components/runs/telemetria-panel";
import { ControlesPanel } from "@/components/runs/control-panel";
import { Minimapa } from "@/components/runs/minimap";

type Posicao = { x: number; y: number };

export default function RunsPage() {
  const [telemetria, setTelemetria] = useState<Telemetria | null>(null);
  const [posicoes, setPosicoes] = useState<Posicao[]>([]);

  useEffect(() => {
    async function fetchTelemetria() {
      const novasTelemetria = mockTelemetria;
      setTelemetria(novasTelemetria);
      setPosicoes((anterior) => {
        const posicaoJaVisitada = anterior.some(
          (p) =>
            p.x === novasTelemetria.posicao_x &&
            p.y === novasTelemetria.posicao_y,
        );
        if (posicaoJaVisitada) return anterior;
        return [
          ...anterior,
          { x: novasTelemetria.posicao_x, y: novasTelemetria.posicao_y },
        ];
      });
    }

    fetchTelemetria();
    const intervalo = setInterval(fetchTelemetria, 500);
    return () => clearInterval(intervalo);
  }, []);

  if (!telemetria) return <p>Carregando telemetria...</p>;

  return (
    <div className="grid grid-cols-2 gap-8">
      <div className="flex flex-col gap-4">
        <div>
          <p className="text-sm text-muted-foreground">
            Tipo de labirinto: 16x16
          </p>
        </div>
        {/* Passamos o tamanho do labirinto e o histórico de posições */}
        <Minimapa
          tamanho={16}
          posicoes={posicoes}
          posicaoAtual={{ x: telemetria.posicao_x, y: telemetria.posicao_y }}
        />
      </div>
      <div className="flex flex-col gap-6">
        <TelemetriaPanel telemetria={telemetria} />
        <ControlesPanel telemetria={telemetria} />
      </div>
    </div>
  );
}
