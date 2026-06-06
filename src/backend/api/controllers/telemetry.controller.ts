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
};