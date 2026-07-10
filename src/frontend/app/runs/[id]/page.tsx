import { TelemetriaPanel } from "@/components/runs/telemetria-panel";
import { Minimapa } from "@/components/runs/minimap";
import { backendHttp } from "@/lib/backend";
import { formatarTempo } from "@/lib/utils";

type Props = {
  params: Promise<{
    id: string;
  }>;
};

type CorridaHistorica = {
  id: string;
  status: string;
  tamanhoLabirinto?: number | null;
  tempoConclusaoMs?: number | null;
  velocidadeMedia?: number | null;
  consumoBateriaPct?: number | null;
  desafioCumprido?: boolean | null;
};

export default async function CorridaHistoricaPage({ params }: Props) {
  const { id } = await params;

  const [runResponse, telemetriesResponse] = await Promise.all([
    fetch(`${backendHttp()}/api/telemetria/runs/${id}`, {
      cache: "no-store",
    }),
    fetch(`${backendHttp()}/api/telemetria/runs/${id}/telemetries`, {
      cache: "no-store",
    }),
  ]);

  if (!runResponse.ok) {
    throw new Error("Erro ao buscar corrida");
  }

  if (!telemetriesResponse.ok) {
    throw new Error("Erro ao buscar telemetrias");
  }

  const corrida = await runResponse.json();
  const telemetrias = await telemetriesResponse.json();
  const corridaHistorica = corrida as CorridaHistorica;
  const tamanhoLabirinto = corridaHistorica.tamanhoLabirinto ?? 16;

  if (!telemetrias.length) {
    return (
      <div>
        <h1 className="text-2xl font-bold">Corrida #{id}</h1>

        <p className="text-muted-foreground">
          Nenhuma telemetria encontrada para esta corrida.
        </p>
      </div>
    );
  }

  const ultimaTelemetria = telemetrias[telemetrias.length - 1];

  return (
    <div className="flex flex-col gap-6">
      <div>
        <h1 className="text-2xl font-bold">Corrida #{corrida.id}</h1>

        <p className="text-sm text-muted-foreground">
          Status: {corrida.status}
        </p>
      </div>

      <div className="grid grid-cols-2 gap-4 rounded-lg border p-4 text-sm">
        <div>
          <p className="text-muted-foreground">Tamanho do labirinto</p>
          <p className="font-semibold">
            {tamanhoLabirinto} x {tamanhoLabirinto}
          </p>
        </div>

        <div>
          <p className="text-muted-foreground">Tempo de conclusão</p>
          <p className="font-semibold">
            {corridaHistorica.tempoConclusaoMs != null
              ? formatarTempo(corridaHistorica.tempoConclusaoMs)
              : "-"}
          </p>
        </div>

        <div>
          <p className="text-muted-foreground">Velocidade média</p>
          <p className="font-semibold">
            {corridaHistorica.velocidadeMedia != null
              ? `${corridaHistorica.velocidadeMedia.toFixed(2)} células/s`
              : "-"}
          </p>
        </div>

        <div>
          <p className="text-muted-foreground">Consumo de bateria</p>
          <p className="font-semibold">
            {corridaHistorica.consumoBateriaPct != null
              ? `${corridaHistorica.consumoBateriaPct}%`
              : "-"}
          </p>
        </div>

        <div>
          <p className="text-muted-foreground">Desafio cumprido</p>
          <p className="font-semibold">
            {corridaHistorica.desafioCumprido == null
              ? "-"
              : corridaHistorica.desafioCumprido
                ? "Sim"
                : "Não"}
          </p>
        </div>
      </div>

      <div className="grid grid-cols-2 gap-8">
        <div>
          <Minimapa tamanho={tamanhoLabirinto} telemetries={telemetrias} />
        </div>

        <div className="flex flex-col gap-6">
          <TelemetriaPanel telemetria={ultimaTelemetria} />
        </div>
      </div>
    </div>
  );
}
