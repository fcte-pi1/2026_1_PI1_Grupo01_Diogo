import express from "express";
import cors from "cors";
import { telemetryRoutes } from "./routes/telemetry.routes";

const app = express();

app.use(cors(), express.json());

// Rota ajustada para o português conforme o contrato
app.use("/api/telemetria", telemetryRoutes);

// Porta ajustada para 3000 conforme o contrato
app.listen(3000, () => console.log("Servidor online: http://localhost:3000"));