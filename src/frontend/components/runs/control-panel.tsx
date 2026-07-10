"use client";

import { Button } from "@/components/ui/button";
import { Play, Pause } from "lucide-react";

type Props = {
  corridaEmAndamento: boolean;
  carregando?: boolean;
  onIniciar: () => Promise<void> | void;
  onParar: () => Promise<void> | void;
};

export function ControlesPanel({
  corridaEmAndamento,
  carregando = false,
  onIniciar,
  onParar,
}: Props) {
  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-2xl font-bold">
        Controles
      </h2>

      <Button
        onClick={onIniciar}
        disabled={corridaEmAndamento || carregando}
        className="bg-green-600 hover:bg-green-700 text-white"
      >
        <Play className="h-4 w-4 mr-2" />
        Iniciar Gravação
      </Button>

      <Button
        variant="outline"
        onClick={onParar}
        disabled={!corridaEmAndamento || carregando}
        className="border-red-500 text-red-600 hover:bg-red-50"
      >
        <Pause className="h-4 w-4 mr-2" />
        Parar Gravação
      </Button>
    </div>
  );
}