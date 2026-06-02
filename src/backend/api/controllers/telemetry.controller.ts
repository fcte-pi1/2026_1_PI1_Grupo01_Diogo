import { Request, Response } from "express";
import { TelemetryService } from "../services/telemetry.service";

export const TelemetryController = {
  async create(req: Request, res: Response) {
    try {
      const result = await TelemetryService.save(req.body);
      res.status(201).json(result);
    } catch (e) {
      console.error("❌ Erro detalhado no Controller:", e); // Isso vai nos dizer o que quebrou
      res.status(500).json({ error: "Erro interno" });
    }
  },

  async getLatest(req: Request, res: Response) {
    try {
      res.json((await TelemetryService.getLatest()) || {});
    } catch (e) {
      res.status(500).json({ error: "Erro interno" });
    }
  }
};