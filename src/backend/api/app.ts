import express from "express";
import cors from "cors";
import { telemetryRoutes } from "./routes/telemetry.routes";

export const app = express();

app.use(cors(), express.json());
app.use("/api/telemetria", telemetryRoutes);
