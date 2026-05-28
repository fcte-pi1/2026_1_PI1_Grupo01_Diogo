"use client";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { useRouter } from "next/navigation";
import { Button } from "@/components/ui/button";
import { useCorridaContext } from "@/lib/run-context";
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
  AlertDialogTrigger,
} from "@/components/ui/alert-dialog";
import { TriangleAlert } from "lucide-react";

const navConfig: Record<string, { label: string; href: string }[]> = {
  "/": [
    { label: "Nova Corrida", href: "/runs" },
    { label: "Atual Corrida", href: "/runs" },
  ],
  "/runs": [{ label: "Histórico de Corridas", href: "/" }],
};

export function Navbar() {
  const { corridaEmAndamento } = useCorridaContext();
  const pathname = usePathname();
  const router = useRouter();

  const navItems =
    navConfig[pathname] ??
    Object.entries(navConfig).find(
      ([key]) => pathname.startsWith(key) && key !== "/",
    )?.[1] ??
    [];

  function handleConfirmarNovaCorreida() {
    // Rikas - Por enquanto só navega — quando a API estiver pronta,
    // aqui você vai: 1) salvar corrida atual, 2) criar nova, 3) navegar
    console.log("Nova corrida confirmada");
    router.push("/runs");
  }

  return (
    <header className="flex items-center justify-between px-6 py-4 border-b">
      <Link href="/" className="flex items-center gap-2 font-bold text-lg">
        MrBombastic
      </Link>

      <nav className="flex items-center gap-2">
        {navItems.map((item) => {
          if (item.label === "Nova Corrida") {
            return (
              <AlertDialog key={item.label}>
                <AlertDialogTrigger asChild>
                  <Button variant="ghost">{item.label}</Button>
                </AlertDialogTrigger>
                <AlertDialogContent>
                  <AlertDialogHeader>
                    <AlertDialogMedia>
                      <TriangleAlert />
                    </AlertDialogMedia>
                    <AlertDialogTitle>
                      Iniciar uma nova corrida?
                    </AlertDialogTitle>
                    <AlertDialogDescription>
                      A corrida atual será salva no histórico e uma nova será
                      iniciada.
                    </AlertDialogDescription>
                  </AlertDialogHeader>
                  <AlertDialogFooter>
                    <AlertDialogCancel>Cancelar</AlertDialogCancel>
                    <AlertDialogAction onClick={handleConfirmarNovaCorreida}>
                      Confirmar
                    </AlertDialogAction>
                  </AlertDialogFooter>
                </AlertDialogContent>
              </AlertDialog>
            );
          }
          if (item.label === "Atual Corrida") {
            return (
              <Button
                key={item.label}
                variant="ghost"
                disabled={!corridaEmAndamento}
                onClick={() => router.push(item.href)}
              >
                {item.label}
              </Button>
            );
          }
          return (
            <Button key={item.label} variant="ghost" asChild>
              <Link href={item.href}>{item.label}</Link>
            </Button>
          );
        })}
      </nav>
    </header>
  );
}
