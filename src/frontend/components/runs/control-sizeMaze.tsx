"use client";

import { Tabs, TabsList, TabsTrigger } from "@/components/ui/tabs";

type Props = {
  tamanho: 4 | 8 | 16;
  onChange: (tamanho: 4 | 8 | 16) => void;
  desabilitado?: boolean;
};

export function SelecaoLabirinto({ tamanho, onChange, desabilitado }: Props) {
  return (
    <div className="flex flex-col gap-2">
      <p className="text-sm text-muted-foreground">Tipo de labirinto</p>
      <Tabs
        value={String(tamanho)}
        onValueChange={(v) => onChange(Number(v) as 4 | 8 | 16)}
      >
        <TabsList>
          <TabsTrigger value="4" disabled={desabilitado}>
            4x4
          </TabsTrigger>
          <TabsTrigger value="8" disabled={desabilitado}>
            8x8
          </TabsTrigger>
          <TabsTrigger value="16" disabled={desabilitado}>
            16x16
          </TabsTrigger>
        </TabsList>
      </Tabs>
    </div>
  );
}
