-- CreateTable
CREATE TABLE "runs" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "status" TEXT NOT NULL DEFAULT 'EM_ANDAMENTO',
    "startedAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "endedAt" DATETIME
);

-- CreateTable
CREATE TABLE "telemetries" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "run_id" TEXT NOT NULL,
    "tempo_corrida_ms" INTEGER NOT NULL,
    "posicao_x" INTEGER NOT NULL,
    "posicao_y" INTEGER NOT NULL,
    "direcao_atual" TEXT NOT NULL,
    "estado_robo" TEXT NOT NULL,
    "bateria_pct" INTEGER NOT NULL,
    "dist_frente_cm" REAL NOT NULL,
    "dist_esquerda_cm" REAL NOT NULL,
    "dist_direita_cm" REAL NOT NULL,
    "timestamp" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "telemetries_run_id_fkey" FOREIGN KEY ("run_id") REFERENCES "runs" ("id") ON DELETE CASCADE ON UPDATE CASCADE
);
