"use client";

import { useState, useEffect } from "react";
import { Telemetria } from "@/lib/MockFakeData";
import { mockTelemetria } from "@/lib/MockFakeData";
import { TelemetriaPanel } from "@/components/runs/telemetria-panel";
import { ControlesPanel } from "@/components/runs/control-panel";
import { SelecaoLabirinto } from "@/components/runs/control-sizeMaze";
import { Minimapa } from "@/components/runs/minimap";
import { Separator } from "@/components/ui/separator";

type Posicao = { x: number; y: number };

export default function RunsPage() {
  const [tamanhoLabirinto, setTamanhoLabirinto] = useState<4 | 8 | 16>(16);
  const [telemetria, setTelemetria] = useState<Telemetria | null>(null);
  const [posicoes, setPosicoes] = useState<Posicao[]>([]);
  const robotAtivo =
    telemetria?.estado_robo === "EXPLORANDO" ||
    telemetria?.estado_robo === "VOLTANDO";

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
        <SelecaoLabirinto
          tamanho={tamanhoLabirinto}
          onChange={setTamanhoLabirinto}
          desabilitado={robotAtivo} // bloqueia a seleção durante a corrida
        />
        <Minimapa
          tamanho={tamanhoLabirinto} // agora é dinâmico!
          posicoes={posicoes}
          posicaoAtual={{ x: telemetria.posicao_x, y: telemetria.posicao_y }}
        />
      </div>
      <div className="flex flex-col gap-6">
        <TelemetriaPanel telemetria={telemetria} />
        <Separator />
        <ControlesPanel telemetria={telemetria} />
      </div>
    </div>
  );
}
