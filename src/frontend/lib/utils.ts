import { clsx, type ClassValue } from "clsx";
import { twMerge } from "tailwind-merge";

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs));
}

// Converte milissegundos para HH:MM:SS
export function formatarTempo(ms: number): string {
  const totalSegundos = Math.floor(ms / 1000);
  const horas = Math.floor(totalSegundos / 3600);
  const minutos = Math.floor((totalSegundos % 3600) / 60);
  const segundos = totalSegundos % 60;

  return [horas, minutos, segundos]
    .map((v) => String(v).padStart(2, "0"))
    .join(":");
}

// Traduz o estado técnico do robô para linguagem amigável ao usuário.
// Aceita qualquer string vinda do backend; estados desconhecidos são
// exibidos como vieram (fallback) para não quebrar a interface.
export function traduzirEstado(estado: string): string {
  const traducoes: Record<string, string> = {
    EXPLORANDO: "Em andamento",
    VOLTANDO: "Em andamento",
    PARADO: "Pausado",
    ERRO: "Erro",
    CONCLUIDO: "Concluído",
    OBJETIVO_ENCONTRADO: "Objetivo encontrado",
  };
  return traducoes[estado] ?? estado;
}
