import { Request, Response } from "express";
import { TelemetryService } from "../services/telemetry.service";

export const TelemetryController = {
  async create(req: Request, res: Response): Promise<void> {
    try {
      const payload = req.body;

      // Validação do Contrato (HTTP 400)
      if (
        payload.tempo_corrida_ms === undefined ||
        payload.posicao_x === undefined ||
        payload.posicao_y === undefined ||
        payload.bateria_pct === undefined
      ) {
        res.status(400).json({
          error: "Bad Request: Payload malformado ou campos obrigatórios ausentes."
        });
        return; // Retorna para interromper a execução aqui mesmo
      }

      const result = await TelemetryService.save(payload);
      res.status(201).json(result);
    } catch (e) {
      console.error("❌ Erro detalhado no Controller:", e);
      res.status(500).json({ error: "Internal Server Error" });
    }
  },

  async getLatest(req: Request, res: Response): Promise<void> {
    try {
      res.json((await TelemetryService.getLatest()) || {});
    } catch (e) {
      console.error("❌ Erro no getLatest:", e);
      res.status(500).json({ error: "Internal Server Error" });
    }
  },

  async getRuns(req: Request, res: Response): Promise<void> {
    try {
      const runs = await TelemetryService.getRuns();
      res.json(runs);
    } catch (e) {
      console.error("❌ Erro ao buscar corridas:", e);
      res.status(500).json({
        error: "Internal Server Error",
      });
    }
  },
  async getRunById(req: Request, res: Response): Promise<void> {
    try {
     
      const { id } = req.params as { id: string };

      const run = await TelemetryService.getRunById(id);

      if (!run) {
        res.status(404).json({
          error: "Corrida não encontrada",
        });
        return;
      }

      res.json(run);
    } catch (e) {
      console.error(e);

      res.status(500).json({
        error: "Internal Server Error",
      });
    }
  },

  async getTelemetriesByRunId(
  req: Request,
  res: Response
): Promise<void> {
  try {
    const { id } = req.params as { id: string };

    const telemetries =
      await TelemetryService.getTelemetriesByRunId(id);

    res.json(telemetries);
  } catch (e) {
    console.error(e);

    res.status(500).json({
      error: "Internal Server Error",
    });
  }
},

  // Finaliza manualmente uma corrida (ex.: botão "Parar" no frontend).
  // Idempotente: chamar de novo numa corrida já finalizada não faz nada.
  async finalizarRun(req: Request, res: Response): Promise<void> {
    try {
      const { id } = req.params as { id: string };

      const run = await TelemetryService.getRunById(id);
      if (!run) {
        res.status(404).json({ error: "Corrida não encontrada" });
        return;
      }

      await TelemetryService.finalizeRun(id, "NAO_CONCLUIDA");
      const runAtualizada = await TelemetryService.getRunById(id);
      res.status(200).json(runAtualizada);
    } catch (e) {
      console.error("❌ Erro ao finalizar corrida:", e);
      res.status(500).json({ error: "Internal Server Error" });
    }
  },

  // Remove uma corrida e suas telemetrias.
  async deleteRun(req: Request, res: Response): Promise<void> {
    try {
      const { id } = req.params as { id: string };

      const run = await TelemetryService.getRunById(id);
      if (!run) {
        res.status(404).json({ error: "Corrida não encontrada" });
        return;
      }

      await TelemetryService.deleteRun(id);
      res.status(204).send();
    } catch (e) {
      console.error("❌ Erro ao deletar corrida:", e);
      res.status(500).json({ error: "Internal Server Error" });
    }
  },
};