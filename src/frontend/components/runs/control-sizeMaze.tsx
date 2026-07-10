"use client";

import {
  Tabs,
  TabsList,
  TabsTrigger,
} from "@/components/ui/tabs";

type MazeSize = 4 | 8 | 16;

type Props = {
  tamanho: MazeSize;
  onChange: (tamanho: MazeSize) => void;
  desabilitado?: boolean;
};

export function SelecaoLabirinto({
  tamanho,
  onChange,
  desabilitado = false,
}: Props) {
  return (
    <div className="flex flex-col gap-2">
      <p className="text-sm text-muted-foreground">
        Tamanho do labirinto
      </p>

      <Tabs
        value={String(tamanho)}
        onValueChange={(value) =>
          onChange(Number(value) as MazeSize)
        }
      >
        <TabsList>
          <TabsTrigger
            value="4"
            disabled={desabilitado}
          >
            4x4
          </TabsTrigger>

          <TabsTrigger
            value="8"
            disabled={desabilitado}
          >
            8x8
          </TabsTrigger>

          <TabsTrigger
            value="16"
            disabled={desabilitado}
          >
            16x16
          </TabsTrigger>
        </TabsList>
      </Tabs>
    </div>
  );
}