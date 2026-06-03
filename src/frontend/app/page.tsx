import { CorridasTable } from "@/components/history/table-runs";
import { mockCorridas } from "@/lib/MockFakeData";

export default function HistoryPage() {
  return (
    <div>
      <h1 className="text-2xl font-bold mb-6">Banco de Corridas</h1>
      <CorridasTable corridas={mockCorridas} />
    </div>
  );
}