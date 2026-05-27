"use client";

import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";
import { Button } from "@/components/ui/button";
import { Trash2 } from "lucide-react";
type Corrida = {
  id: number;
  nome: string;
  index: number;
};
export function CorridasTable({ corridas }: { corridas: Corrida[] }) {
  function handleApagar(id: number) {
    // Rikas - Por enquanto só loga no console — integraremos com a API depois
    console.log("Apagar corrida:", id);
  }

  return (
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
                onClick={() => handleApagar(corrida.id)}
              >
                <Trash2 className="h-4 w-4" />
              </Button>
            </TableCell>
          </TableRow>
        ))}
      </TableBody>
    </Table>
  );
}
