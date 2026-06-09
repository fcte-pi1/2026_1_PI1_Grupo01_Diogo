import { TelemetriaPanel } from "@/components/runs/telemetria-panel";
import { Minimapa } from "@/components/runs/minimap";

type Props = {
  params: Promise<{
    id: string;
  }>;
};

export default async function CorridaHistoricaPage({
  params,
}: Props) {
  const { id } = await params;

  const [runResponse, telemetriesResponse] =
    await Promise.all([
      fetch(
        `http://localhost:3000/api/telemetria/runs/${id}`,
        {
          cache: "no-store",
        }
      ),
      fetch(
        `http://localhost:3000/api/telemetria/runs/${id}/telemetries`,
        {
          cache: "no-store",
        }
      ),
    ]);

  if (!runResponse.ok) {
    throw new Error("Erro ao buscar corrida");
  }

  if (!telemetriesResponse.ok) {
    throw new Error("Erro ao buscar telemetrias");
  }

  const corrida = await runResponse.json();
  const telemetrias = await telemetriesResponse.json();

  if (!telemetrias.length) {
    return (
      <div>
        <h1 className="text-2xl font-bold">
          Corrida #{id}
        </h1>

        <p className="text-muted-foreground">
          Nenhuma telemetria encontrada para esta corrida.
        </p>
      </div>
    );
  }

  const ultimaTelemetria =
    telemetrias[telemetrias.length - 1];

  return (
    <div className="flex flex-col gap-6">
      <div>
        <h1 className="text-2xl font-bold">
          Corrida #{corrida.id}
        </h1>

        <p className="text-sm text-muted-foreground">
          Status: {corrida.status}
        </p>
      </div>

      <div className="grid grid-cols-2 gap-8">
        <div>
          <Minimapa
            tamanho={16}
            telemetries={telemetrias}
          />
        </div>

        <div className="flex flex-col gap-6">
          <TelemetriaPanel
            telemetria={ultimaTelemetria}
          />
        </div>
      </div>
    </div>
  );
}