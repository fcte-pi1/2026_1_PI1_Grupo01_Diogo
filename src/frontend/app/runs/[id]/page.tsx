import { TelemetriaPanel } from "@/components/runs/telemetria-panel";
import { Minimapa } from "@/components/runs/minimap";
import { mockTelemetria, mockPosicoes } from "@/lib/MockFakeData";

type Props = {
  params: { id: string };
};
export default async function CorridaHistoricaPage({ params }: Props) {
  // Rikas - Futuramente: const corrida = await fetch(`/api/corridas/${params.id}`)
  const { id } = await params;
  const corrida = mockTelemetria;
  const ultimaPosicao = mockPosicoes[mockPosicoes.length - 1];
  return (
    <div className="flex flex-col gap-6">
      <div>
        <h1 className="text-2xl font-bold">Corrida #{id}</h1>
        <p className="text-sm text-muted-foreground">
          Visualização histórica — somente leitura
        </p>
      </div>

      <div className="grid grid-cols-2 gap-8">
        <div>
          <Minimapa
            tamanho={16}
            posicoes={mockPosicoes}
            posicaoAtual={ultimaPosicao}
          />
        </div>

        <div className="flex flex-col gap-6">
          <TelemetriaPanel telemetria={corrida} />
        </div>
      </div>
    </div>
  );
}
