// ============================================================
// Protocolo de mensagens do WebSocket.
//
// Toda mensagem trafega num "envelope" padrão: { type, payload }.
//   - `type`    identifica o tipo da mensagem (telemetria, comando, ping...).
//   - `payload` carrega os dados específicos daquele tipo.
//
// A leitura é TOLERANTE: mensagens sem envelope (objeto cru) são tratadas como
// telemetria, para manter compatibilidade com clientes que ainda enviam o
// objeto direto (robô/simulador atuais).
// ============================================================

export type Envelope = {
  type: string;
  payload?: unknown;
};

// Constrói o envelope serializado para envio.
export function envelope(type: string, payload?: unknown): string {
  return JSON.stringify({ type, payload });
}

// Interpreta uma mensagem recebida. Lança se o JSON for inválido.
export function parseMensagem(raw: string): Envelope {
  const obj = JSON.parse(raw);

  if (
    obj &&
    typeof obj === "object" &&
    typeof (obj as { type?: unknown }).type === "string"
  ) {
    const comEnvelope = obj as { type: string; payload?: unknown };
    return { type: comEnvelope.type, payload: comEnvelope.payload };
  }

  // Sem envelope → assume telemetria (compatibilidade retroativa).
  return { type: "telemetria", payload: obj };
}
