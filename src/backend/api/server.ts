import express from "express";
import cors from "cors";
import { telemetryRoutes } from "./routes/telemetry.routes";

const app = express();

app.use(cors(), express.json());
app.use("/api/telemetry", telemetryRoutes);

app.listen(3333, () => console.log(" Servidor online: http://localhost:3333"));
