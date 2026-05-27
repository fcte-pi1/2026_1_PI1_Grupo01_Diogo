"use client";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { Button } from "@/components/ui/button";

const navConfig: Record<string, { label: string; href: string }[]> = {
  "/": [
    { label: "Nova Corrida", href: "/runs" },
    { label: "Atual Corrida", href: "/runs" },
  ],
  "/runs": [{ label: "Histórico de Corridas", href: "/" }],
};

export function Navbar() {
  const pathname = usePathname();
  const navItems =
    navConfig[pathname] ??
    Object.entries(navConfig).find(
      ([key]) => pathname.startsWith(key) && key !== "/",
    )?.[1] ??
    [];

  return (
    <header className="flex items-center justify-between px-6 py-4 border-b">
      <Link href="/" className="flex items-center gap-2 font-bold text-lg">
        MrBombastic
      </Link>
      <nav className="flex items-center gap-2">
        {navItems.map((item) => (
          <Button key={item.label} variant="ghost" asChild>
            <Link href={item.href}>{item.label}</Link>
          </Button>
        ))}
      </nav>
    </header>
  );
}
