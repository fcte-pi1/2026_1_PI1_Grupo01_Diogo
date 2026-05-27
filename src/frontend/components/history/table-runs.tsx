"use client";
import { useState } from "react";
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
  AlertDialogMedia, // ← adicionar aqui
  AlertDialogTitle,
} from "@/components/ui/alert-dialog";
import { Trash2, TriangleAlert } from "lucide-react";
type Corrida = {
  id: number;
  nome: string;
  index: number;
};
export function CorridasTable({ corridas }: { corridas: Corrida[] }) {
  const [corridaParaApagar, setCorridaParaApagar] = useState<Corrida | null>(
    null,
  );
  function handleConfirmarApagar() {
    // Rikas - Por enquanto só loga — integraremos com a API depois
    console.log("Apagar corrida:", corridaParaApagar?.id);
    setCorridaParaApagar(null);
  }
  return (
    <>
      <Table>
        <TableHeader>
          <TableRow>
            <TableHead>Nome</TableHead>
            <TableHead>Index</TableHead>
            <TableHead>Apagar</TableHead>
          </TableRow>
        </TableHeader>
        <TableBody>
          {corridas.map((corrida) => (
            <TableRow key={corrida.id}>
              <TableCell>{corrida.nome}</TableCell>
              <TableCell>{corrida.index}</TableCell>
              <TableCell>
                <Button
                  variant="ghost"
                  size="icon"
                  onClick={() => setCorridaParaApagar(corrida)}
                >
                  <Trash2 className="h-4 w-4" />
                </Button>
              </TableCell>
            </TableRow>
          ))}
        </TableBody>
      </Table>
      <AlertDialog
        open={corridaParaApagar !== null}
        onOpenChange={(open) => !open && setCorridaParaApagar(null)}
      >
        <AlertDialogContent>
          <AlertDialogHeader>
            <AlertDialogMedia>
              <TriangleAlert />
            </AlertDialogMedia>
            <AlertDialogTitle>
              Deseja apagar a corrida "{corridaParaApagar?.index}"?
            </AlertDialogTitle>
            <AlertDialogDescription>
              Essa ação não pode ser desfeita.
            </AlertDialogDescription>
          </AlertDialogHeader>
          <AlertDialogFooter>
            <AlertDialogCancel onClick={() => setCorridaParaApagar(null)}>
              Não
            </AlertDialogCancel>
            <AlertDialogAction onClick={handleConfirmarApagar}>
              Sim
            </AlertDialogAction>
          </AlertDialogFooter>
        </AlertDialogContent>
      </AlertDialog>
    </>
  );
}
