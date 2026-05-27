import { CorridasTable } from "@/components/history/table-runs";
import { mockCorridas } from "@/lib/MockFakeData";

export default function HistoryPage() {
  return (
    <main className="container mx-auto py-8">
      <h1 className="text-2xl font-bold mb-6">Banco de Corridas</h1>
      {/* Rikas - os dados mockados como prop — quando a API estiver pronta
          substituímos mockfake por uma chamada fetch() aqui */}
      <CorridasTable corridas={mockCorridas} />
    </main>
  );
}
