-- AlterTable: métricas agregadas da corrida + tamanho do labirinto (todos opcionais)
ALTER TABLE "runs" ADD COLUMN "tempo_conclusao_ms" INTEGER;
ALTER TABLE "runs" ADD COLUMN "velocidade_media" REAL;
ALTER TABLE "runs" ADD COLUMN "consumo_bateria_pct" INTEGER;
ALTER TABLE "runs" ADD COLUMN "desafio_cumprido" BOOLEAN;
ALTER TABLE "runs" ADD COLUMN "trajeto_coordenadas" TEXT;
ALTER TABLE "runs" ADD COLUMN "tamanho_labirinto" INTEGER;
