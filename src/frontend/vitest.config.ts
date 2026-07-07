import { defineConfig } from "vitest/config";
import react from "@vitejs/plugin-react";
import path from "path";

export default defineConfig({
  plugins: [react()],
  test: {
    environment: "jsdom",
    setupFiles: ["./vitest.setup.ts"],
    css: true,
    coverage: {
      provider: "v8",
      reporter: ["text", "html", "lcov"],
      include: ["components/**/*.{ts,tsx}", "lib/**/*.{ts,tsx}"],
      exclude: [
        "components/ui/**", // componentes shadcn/ui prontos, não são lógica nossa
        "**/*.d.ts",
        "**/*.test.{ts,tsx}", // não conta os próprios arquivos de teste como cobertura
        "lib/FakeTips.ts", // apenas definições de tipos, sem lógica em runtime
        "lib/MockFakeData.ts", // dados estáticos de placeholder para desenvolvimento
      ],
      thresholds: {
        lines: 70,
        functions: 70,
        branches: 60,
        statements: 70,
      },
    },
  },
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "."),
    },
  },
});
