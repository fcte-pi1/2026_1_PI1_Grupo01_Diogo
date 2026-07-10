import type { NextConfig } from "next";
import path from "path";

const nextConfig: NextConfig = {
  // Fixa a raiz do Turbopack NESTE diretório (src/frontend). Sem isso, o Next
  // infere a raiz como `src/` (há package-lock.json em backend/ e frontend/) e
  // o dev server passa a vigiar a árvore inteira de `src/` (~70k arquivos:
  // backend/node_modules, firmware/.pio, etc.) — o que estoura a memória e
  // trava/derruba o `next dev` em qualquer máquina.
  turbopack: {
    root: path.join(__dirname),
  },
};

export default nextConfig;
