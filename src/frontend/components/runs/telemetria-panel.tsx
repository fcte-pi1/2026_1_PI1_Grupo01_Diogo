"use client";

import {
  Battery,
  Timer,
  MapPin,
  Compass,
  Radar,
} from "lucide-react";

import {
  formatarTempo,
  traduzirEstado,
} from "@/lib/utils";

export type TelemetriaUI = {
  id: string;
  runId: string;

  tempoCorridaMs: number;

  posicaoX: number;
  posicaoY: number;

  direcaoAtual: string;

  // String livre: o backend pode enviar estados fora da união (ex.: OBJETIVO_ENCONTRADO).
  estadoRobo: string;

  bateriaPct: number;

  distFrenteCm: number;
  distEsquerdaCm: number;
  distDireitaCm: number;
};

export function TelemetriaPanel({
  telemetria,
}: {
  telemetria: TelemetriaUI;
}) {
  return (
    <div className="flex flex-col gap-6">
      <div>
        <h2 className="text-2xl font-bold">
          Informações
        </h2>
      </div>

      {/* Estado */}
      <div className="border rounded-lg p-4">
        <p className="text-xs text-muted-foreground">
          Estado Atual
        </p>

        <p className="font-bold">
          {traduzirEstado(
            telemetria.estadoRobo
          )}
        </p>
      </div>

      {/* Bateria e Tempo */}
      <div className="grid grid-cols-2 gap-4">
        <div className="flex items-center gap-3 border rounded-lg p-4">
          <Battery className="h-5 w-5" />

          <div>
            <p className="text-xs text-muted-foreground">
              Bateria
            </p>

            <p className="font-bold">
              {telemetria.bateriaPct}%
            </p>
          </div>
        </div>

        <div className="flex items-center gap-3 border rounded-lg p-4">
          <Timer className="h-5 w-5" />

          <div>
            <p className="text-xs text-muted-foreground">
              Tempo
            </p>

            <p className="font-bold">
              {formatarTempo(
                telemetria.tempoCorridaMs
              )}
            </p>
          </div>
        </div>
      </div>

      {/* Posição */}
      <div className="flex items-center gap-3 border rounded-lg p-4">
        <MapPin className="h-5 w-5" />

        <div>
          <p className="text-xs text-muted-foreground">
            Posição
          </p>

          <p className="font-bold">
            ({telemetria.posicaoX},{" "}
            {telemetria.posicaoY})
          </p>
        </div>
      </div>

      {/* Direção */}
      <div className="flex items-center gap-3 border rounded-lg p-4">
        <Compass className="h-5 w-5" />

        <div>
          <p className="text-xs text-muted-foreground">
            Direção
          </p>

          <p className="font-bold">
            {telemetria.direcaoAtual}
          </p>
        </div>
      </div>

      {/* Sensores */}
      <div className="border rounded-lg p-4">
        <div className="flex items-center gap-2 mb-3">
          <Radar className="h-5 w-5" />

          <p className="font-semibold">
            Sensores
          </p>
        </div>

        <div className="grid grid-cols-3 gap-4">
          <div>
            <p className="text-xs text-muted-foreground">
              Frente
            </p>

            <p className="font-bold">
              {telemetria.distFrenteCm.toFixed(1)} cm
            </p>
          </div>

          <div>
            <p className="text-xs text-muted-foreground">
              Esquerda
            </p>

            <p className="font-bold">
              {telemetria.distEsquerdaCm.toFixed(1)} cm
            </p>
          </div>

          <div>
            <p className="text-xs text-muted-foreground">
              Direita
            </p>

            <p className="font-bold">
              {telemetria.distDireitaCm.toFixed(1)} cm
            </p>
          </div>
        </div>
      </div>
    </div>
  );
}