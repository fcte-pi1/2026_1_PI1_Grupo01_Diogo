import type { Metadata } from "next";
import "./globals.css";
import { Navbar } from "@/components/layout/navbar";
import { JetBrains_Mono } from "next/font/google";
import { cn } from "@/lib/utils";

const jetbrainsMono = JetBrains_Mono({subsets:['latin'],variable:'--font-mono'});

export const metadata: Metadata = {
  title: "MrBombastic",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="pt-BR" className={cn("font-mono", jetbrainsMono.variable)}>
      <body className="min-h-screen bg-background text-foreground">
        <Navbar />
        <main className="container mx-auto px-6 py-8">{children}</main>
      </body>
    </html>
  );
}
