// Endereço do backend derivado em runtime.
//
// - No navegador: usa o host que serviu a página (window.location.hostname).
//   Assim o dashboard funciona tanto em http://localhost:3001 quanto acessado
//   pelo IP da rede (ex.: http://192.168.1.204:3001 de outro aparelho), sem
//   apontar "localhost" (que, no outro dispositivo, seria o próprio aparelho).
// - No servidor (SSR / server components): fala consigo mesmo em localhost.
// - Pode ser sobrescrito por NEXT_PUBLIC_BACKEND_HOST (só o host, ex.: 10.0.0.5).

const BACKEND_PORT = 3000;

function backendHost(): string {
  const override = process.env.NEXT_PUBLIC_BACKEND_HOST;
  if (override) return override;
  if (typeof window !== "undefined") return window.location.hostname;
  return "localhost";
}

// Base HTTP do backend, ex.: "http://192.168.1.204:3000".
export function backendHttp(): string {
  return `http://${backendHost()}:${BACKEND_PORT}`;
}

// URL do WebSocket do backend, ex.: "ws://192.168.1.204:3000/ws".
export function backendWs(): string {
  const proto =
    typeof window !== "undefined" && window.location.protocol === "https:"
      ? "wss:"
      : "ws:";
  return `${proto}//${backendHost()}:${BACKEND_PORT}/ws`;
}
