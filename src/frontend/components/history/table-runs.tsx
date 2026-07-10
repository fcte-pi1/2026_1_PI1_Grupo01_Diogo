"use client";
import { useState, useEffect } from "react";
import { useRouter } from "next/navigation";
import { backendHttp } from "@/lib/backend";
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";
import { Button } from "@/components/ui/button";
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogMedia,
  AlertDialogTitle,
} from "@/components/ui/alert-dialog";
import { Trash2, TriangleAlert, RefreshCw } from "lucide-react";

// 1. Tipagem atualizada conforme o seu schema.prisma do modelo Run
type Corrida = {
  id: string; // Mudou de number para string (UUID)
  status: string;
  startedAt: string;
  endedAt: string | null;
};

export function CorridasTable() {
  const router = useRouter();
  
  // Estados para gerenciar as corridas do banco, carregamento e erros
  const [listaCorridas, setListaCorridas] = useState<Corrida[]>([]);
  const [loading, setLoading] = useState<boolean>(true);
  const [corridaParaApagar, setCorridaParaApagar] = useState<Corrida | null>(null);

  // URL base da sua API Express
  const API_URL = `${backendHttp()}/api/telemetria/runs`;

  // 2. Função para buscar as corridas da API Express
  async function buscarCorridas() {
    try {
      setLoading(true);
      const res = await fetch(API_URL);
      if (!res.ok) throw new Error("Falha ao buscar dados da API");
      const dados = await res.json();
      setListaCorridas(dados);
    } catch (error) {
      console.error("Erro ao carregar corridas:", error);
    } finally {
      setLoading(false);
    }
  }

  // Busca as corridas assim que o componente monta na tela
  useEffect(() => {
    buscarCorridas();
  }, []);

  // 3. Função para apagar a corrida direto no banco via API
  async function handleConfirmarApagar() {
    if (!corridaParaApagar) return;

    try {
      // Faz a requisição DELETE para a API passando o ID da corrida
      const res = await fetch(`${API_URL}/${corridaParaApagar.id}`, {
        method: "DELETE",
      });

      if (res.ok) {
        // Atualiza o estado local removendo a corrida deletada (otimista)
        setListaCorridas((prev) => prev.filter((c) => c.id !== corridaParaApagar.id));
        console.log(`Corrida ${corridaParaApagar.id} deletada com sucesso.`);
      } else {
        console.error("Erro ao deletar no servidor:", res.status);
      }
    } catch (error) {
      console.error("Erro na requisição de deleção:", error);
    } finally {
      setCorridaParaApagar(null);
    }
  }

  // Formatador de data amigável para a tabela
  const formatarData = (dataStr: string) => {
    return new Date(dataStr).toLocaleString("pt-BR", {
      day: "2-digit",
      month: "2-digit",
      year: "numeric",
      hour: "2-digit",
      minute: "2-digit",
    });
  };

  if (loading) {
    return (
      <div className="flex items-center justify-center p-8 gap-2 text-muted-foreground">
        <RefreshCw className="h-5 w-5 animate-spin" />
        <span>Carregando histórico do Micromouse...</span>
      </div>
    );
  }

  return (
    <>
      <Table>
        <TableHeader>
          <TableRow>
            <TableHead>ID da Corrida</TableHead>
            <TableHead>Início</TableHead>
            <TableHead>Status</TableHead>
            <TableHead className="text-right">Ações</TableHead>
          </TableRow>
        </TableHeader>
        <TableBody>
          {listaCorridas.length === 0 ? (
            <TableRow>
              <TableCell colSpan={4} className="text-center p-8 text-muted-foreground">
                Nenhuma corrida registrada no banco ainda.
              </TableCell>
            </TableRow>
          ) : (
            listaCorridas.map((corrida) => (
              <TableRow
                key={corrida.id}
                className="cursor-pointer hover:bg-muted"
                onClick={() => router.push(`/runs/${corrida.id}`)}
              >
                {/* Exibe os primeiros caracteres do UUID para não estourar o layout */}
                <TableCell className="font-mono text-xs">
                  {corrida.id.substring(0, 8)}...
                </TableCell>
                <TableCell>{formatarData(corrida.startedAt)}</TableCell>
                <TableCell>
                  <span className={`px-2 py-1 rounded text-xs font-semibold ${
                    corrida.status === "EM_ANDAMENTO" 
                      ? "bg-yellow-500/10 text-yellow-600" 
                      : "bg-green-500/10 text-green-600"
                  }`}>
                    {corrida.status}
                  </span>
                </TableCell>
                <TableCell className="text-right">
                  <Button
                    variant="ghost"
                    size="icon"
                    onClick={(e) => {
                      e.stopPropagation(); // Evita clicar na linha e abrir o router.push
                      setCorridaParaApagar(corrida);
                    }}
                  >
                    <Trash2 className="h-4 w-4 text-destructive" />
                  </Button>
                </TableCell>
              </TableRow>
            ))
          )}
        </TableBody>
      </Table>

      <AlertDialog
        open={corridaParaApagar !== null}
        onOpenChange={(open) => !open && setCorridaParaApagar(null)}
      >
        <AlertDialogContent>
          <AlertDialogHeader>
            <AlertDialogMedia>
              <TriangleAlert className="text-destructive" />
            </AlertDialogMedia>
            <AlertDialogTitle>
              Deseja apagar a corrida?
            </AlertDialogTitle>
            <AlertDialogDescription>
              A corrida com ID <span className="font-mono">{corridaParaApagar?.id.substring(0, 8)}...</span> e todas as suas telemetrias atreladas serão excluídas permanentemente do SQLite.
            </AlertDialogDescription>
          </AlertDialogHeader>
          <AlertDialogFooter>
            <AlertDialogCancel onClick={() => setCorridaParaApagar(null)}>
              Não
            </AlertDialogCancel>
            <AlertDialogAction onClick={handleConfirmarApagar} className="bg-destructive hover:bg-destructive/90">
              Sim, Apagar
            </AlertDialogAction>
          </AlertDialogFooter>
        </AlertDialogContent>
      </AlertDialog>
    </>
  );
}