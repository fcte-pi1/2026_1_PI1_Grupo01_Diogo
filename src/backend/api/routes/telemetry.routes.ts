import { Router } from "express";
import { TelemetryController } from "../controllers/telemetry.controller";

export const telemetryRoutes = Router();

telemetryRoutes.post("/", TelemetryController.create);
telemetryRoutes.get("/latest", TelemetryController.getLatest);