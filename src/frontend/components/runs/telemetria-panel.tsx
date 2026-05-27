import { Telemetria } from "@/lib/FakeTips";

export function TelemetriaPanel({ telemetria }: { telemetria: Telemetria }) {
  return (
    <div>
      <h2>Informações</h2>
      <p>Bateria: {telemetria.bateria_pct}%</p>
      <p>Velocidade: {telemetria.leitura_sensores.dist_frente_cm}</p>
      <p>Tempo: {telemetria.tempo_corrida_ms}ms</p>
      <p>Estado: {telemetria.estado_robo}</p>
    </div>
  );
}
