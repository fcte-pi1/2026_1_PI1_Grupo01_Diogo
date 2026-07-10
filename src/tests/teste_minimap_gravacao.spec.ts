import { test, expect, Page } from "@playwright/test";

// ============================================================
// E2E — Visualização da corrida e construção do caminho no minimapa
//
// Escopo reduzido (issue #97): o robô salva a corrida (POSTa telemetria);
// a web apenas OBSERVA. Aqui validamos que, ao acompanhar a corrida:
//  1. O caminho é desenhado no minimapa conforme a telemetria chega.
//  2. Quando o robô reporta estado terminal (OBJETIVO_ENCONTRADO), o
//     viewer mostra o objetivo (🏁).
//
// A rede é interceptada (page.route) para o teste ser determinístico
// e independente do backend Express.
// ============================================================

const RUN_ID = "e2e-run-1";

// Caminho percorrido pelo "robô" (uma célula por iteração de polling).
const CAMINHO = [
  { x: 0, y: 0 },
  { x: 1, y: 0 },
  { x: 2, y: 0 },
  { x: 3, y: 0 },
  { x: 3, y: 1 },
  { x: 3, y: 2 },
  { x: 4, y: 2 },
  { x: 5, y: 2 },
];

const CORS = { ...{ "access-control-allow-origin": "*" }, "content-type": "application/json" };

function pontoEm(i: number, encerrarPeloRobo?: boolean) {
  const ehUltimo = i === CAMINHO.length - 1;
  const passo = CAMINHO[i];
  return {
    id: `t-${i}`,
    runId: RUN_ID,
    tempoCorridaMs: i * 500,
    posicaoX: passo.x,
    posicaoY: passo.y,
    direcaoAtual: "LESTE",
    estadoRobo: encerrarPeloRobo && ehUltimo ? "OBJETIVO_ENCONTRADO" : "EXPLORANDO",
    bateriaPct: 90,
    distFrenteCm: 10,
    distEsquerdaCm: 10,
    distDireitaCm: 10,
    timestamp: new Date().toISOString(),
  };
}

async function interceptarApi(page: Page, encerrarPeloRobo = false) {
  // Quantas células já foram reveladas (cresce a cada ciclo de polling).
  let revelados = 1;

  await page.route("**/api/telemetria/**", async (route) => {
    const url = route.request().url();
    const metodo = route.request().method();

    // /runs → lista de corridas; expõe a corrida ativa (EM_ANDAMENTO).
    if (url.endsWith("/runs") && metodo === "GET") {
      await route.fulfill({
        status: 200,
        headers: CORS,
        body: JSON.stringify([
          { id: RUN_ID, status: "EM_ANDAMENTO", startedAt: new Date().toISOString(), endedAt: null },
        ]),
      });
      return;
    }

    // /runs/:id/telemetries → trajetória completa acumulada até agora.
    if (url.includes(`/runs/${RUN_ID}/telemetries`) && metodo === "GET") {
      const qtd = Math.min(revelados, CAMINHO.length);
      const pontos = Array.from({ length: qtd }, (_, i) =>
        pontoEm(i, encerrarPeloRobo)
      );
      revelados = Math.min(revelados + 1, CAMINHO.length);
      await route.fulfill({
        status: 200,
        headers: CORS,
        body: JSON.stringify(pontos),
      });
      return;
    }

    await route.continue();
  });
}

// Conta as células já visitadas (cinza, #888 = rgb(136,136,136)).
const contarVisitadas = (page: Page) =>
  page.evaluate(() =>
    Array.from(document.querySelectorAll("div")).filter(
      (d) => (d as HTMLElement).style.backgroundColor === "rgb(136, 136, 136)"
    ).length
  );

test("desenha o caminho no minimapa ao acompanhar a corrida", async ({
  page,
}) => {
  await interceptarApi(page);

  await page.goto("http://localhost:3001/runs");

  // Antes de acompanhar: nenhuma telemetria, robô fora do mapa.
  await expect(page.getByText("Nenhuma telemetria recebida.")).toBeVisible();
  await expect(page.getByAltText("Micromouse")).toHaveCount(0);

  await page.getByRole("button", { name: /Iniciar Gravação/i }).click();

  // O robô aparece e o caminho cresce no minimapa.
  await expect(page.getByAltText("Micromouse")).toBeVisible();
  await expect
    .poll(() => contarVisitadas(page), {
      message: "o caminho deve crescer no minimapa enquanto acompanha",
      timeout: 8000,
    })
    .toBeGreaterThanOrEqual(3);

  await expect(
    page.getByRole("heading", { name: "Informações" })
  ).toBeVisible();
});

test("mostra o objetivo (🏁) quando o robô reporta estado terminal", async ({
  page,
}) => {
  await interceptarApi(page, true);

  await page.goto("http://localhost:3001/runs");

  await page.getByRole("button", { name: /Iniciar Gravação/i }).click();

  await expect(page.getByAltText("Micromouse")).toBeVisible();

  // Ao chegar OBJETIVO_ENCONTRADO no fim do caminho, o minimapa marca 🏁.
  await expect(page.getByText("🏁")).toBeVisible({ timeout: 8000 });
});
