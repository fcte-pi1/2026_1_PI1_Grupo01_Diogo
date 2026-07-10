import { Router } from "express";
import { TelemetryController } from "../controllers/telemetry.controller";

export const telemetryRoutes = Router();

telemetryRoutes.post("/", TelemetryController.create);
telemetryRoutes.get("/latest", TelemetryController.getLatest);
telemetryRoutes.get("/runs", TelemetryController.getRuns);
telemetryRoutes.get("/runs/:id",TelemetryController.getRunById);
telemetryRoutes.get("/runs/:id/telemetries",TelemetryController.getTelemetriesByRunId);
telemetryRoutes.patch("/runs/:id/finalizar", TelemetryController.finalizarRun);
telemetryRoutes.delete("/runs/:id", TelemetryController.deleteRun);