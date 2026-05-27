import { Telemetria } from "@/lib/FakeTips";
import { Button } from "@/components/ui/button";

export function ControlesPanel({ telemetria }: { telemetria: Telemetria }) {
  return (
    <div>
      <h2>Controles</h2>
      <Button>Iniciar</Button>
      <Button variant="outline">Parar</Button>
    </div>
  );
}
