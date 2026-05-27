"use client";
import Link from "next/link";
import { usePathname } from "next/navigation";
const navConfig = {
  "/": [
    { label: "Nova Corrida", href: "/runs" },
    { label: "Atual Corrida", href: "/" },
  ],
  "/runs": [{ label: "Histórico de Corrida", href: "/" }],
};

export function Navbar() {
  const pathname = usePathname();
  const navItems = navConfig[pathname];
  Object.entries(navConfig).find(
    ([key]) => pathname.startsWith(key) && key !== "/",
  )?.[1] ?? [];

  return (
    <header className="flex items-center justify-between px-6 py-4 border-b">
      <Link href="/" className="flex items-center gap-2 font-bold text-lg">
        MrBombastic
      </Link>
      <nav className="flex items-center gap-2">
        {navItems.map((item) => (
          <Link key={item.href} href={item.href}>
            {item.label}
          </Link>
        ))}
      </nav>
    </header>
  );
}
