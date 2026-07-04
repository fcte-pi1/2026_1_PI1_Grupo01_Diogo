import { createServer } from "http";
import { app } from "./app";
import { attachRealtime } from "./ws/realtime";

const server = createServer(app);
attachRealtime(server);

server.listen(3000, () => console.log("Servidor online: http://localhost:3000"));
