"use client";

import { useState, useEffect } from "react";
import { TelemetriaPanel } from "@/components/runs/telemetria-panel";
import { ControlesPanel } from "@/components/runs/control-panel";
import { SelecaoLabirinto } from "@/components/runs/control-sizeMaze";
import { Minimapa } from "@/components/runs/minimap";
import { Separator } from "@/components/ui/separator";
import { useCorridaContext } from "@/lib/run-context";

type Posicao = { x: number; y: number };

export default function RunsPage() {
  const { telemetria, setCorridaEmAndamento, corridaEmAndamento } = useCorridaContext();
  const [tamanhoLabirinto, setTamanhoLabirinto] = useState<4 | 8 | 16>(16);
  const [posicoes, setPosicoes] = useState<Posicao[]>([]);

  const robotAtivo =
    telemetria?.estadoRobo === "EXPLORANDO" ||
    telemetria?.estadoRobo === "VOLTANDO";

  useEffect(() => {
    if (!telemetria) return;

    // Garante que o rastro só atualiza se as coordenadas forem válidas
    setPosicoes((anterior) => {
      const x = telemetria.posicaoX;
      const y = telemetria.posicaoY;
      
      const posicaoJaVisitada = anterior.some((p) => p.x === x && p.y === y);
      if (posicaoJaVisitada) return anterior;
      
      return [...anterior, { x, y }];
    });
  }, [telemetria]);

  // CÁLCULO DA VELOCIDADE MÉDIA SEGURO
  const tempoSegundos = (telemetria?.tempoCorridaMs || 0) / 1000;
  const totalCelulasVisitadas = posicoes.length || 1;
  const distanciaTotalCm = totalCelulasVisitadas * 18;
  const velocidadeMedia = tempoSegundos > 0 ? (distanciaTotalCm / tempoSegundos) : 0;

  // Criamos o objeto adaptado garantindo fallback (valores padrão) para evitar que a tela suma ou quebre
  const telemetriaAdaptada = {
    id: telemetria?.id || "---",
    runId: telemetria?.runId || "Nenhuma",
    tempo_corrida_ms: telemetria?.tempoCorridaMs || 0,
    posicao_x: telemetria ? Math.min(telemetria.posicaoX, tamanhoLabirinto - 1) : 0, // Evita que a caveira saia do grid
    posicao_y: telemetria ? Math.min(telemetria.posicaoY, tamanhoLabirinto - 1) : 0, // Evita que a caveira saia do grid
    direcao_atual: telemetria?.direcaoAtual || "NORTE",
    estado_robo: telemetria?.estadoRobo || (corridaEmAndamento ? "EXPLORANDO" : "PARADO"),
    bateria_pct: telemetria?.bateriaPct ?? 100,
    velocidade_media: parseFloat(velocidadeMedia.toFixed(2)),
    distFrenteCm: telemetria?.distFrenteCm || 0,
    distEsquerdaCm: telemetria?.distEsquerdaCm || 0,
    distDireitaCm: telemetria?.distDireitaCm || 0
  };

  return (
    <div className="grid grid-cols-2 gap-8">
      <div className="flex flex-col gap-4">
        <SelecaoLabirinto
          tamanho={tamanhoLabirinto}
          onChange={(novoTamanho) => {
            setTamanhoLabirinto(novoTamanho);
            setPosicoes([]); // Limpa o rastro antigo ao mudar o tamanho
          }}
          desabilitado={robotAtivo} 
        />
        <Minimapa
          tamanho={tamanhoLabirinto} 
          posicoes={posicoes}
          posicaoAtual={{ x: telemetriaAdaptada.posicao_x, y: telemetriaAdaptada.posicao_y }}
        />
      </div>
      <div className="flex flex-col gap-6">
        <TelemetriaPanel telemetria={telemetriaAdaptada} />
        <Separator />
        {/* Passamos também a função de alterar o estado para os botões responderem */}
        {/* Passamos as funções reais para os botões do painel */}
        <ControlesPanel 
          corridaEmAndamento={corridaEmAndamento} 
          onIniciar={() => {
            setPosicoes([]); 
            setCorridaEmAndamento(true); 
          }}
          onParar={() => {
            setCorridaEmAndamento(false); 
          }}
        />
      </div>
    </div>
  );
}