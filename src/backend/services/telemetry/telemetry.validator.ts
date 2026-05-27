import type { TelemetryCreateInput, TelemetryValidationResult } from "./telemetry.types";

type JsonObject = Record<string, unknown>;

function isObject(value: unknown): value is JsonObject {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

function optionalString(body: JsonObject, key: string, errors: string[]) {
  const value = body[key];

  if (value === undefined || value === null) {
    return undefined;
  }

  if (typeof value !== "string" || value.trim().length === 0) {
    errors.push(`${key} deve ser uma string nao vazia.`);
    return undefined;
  }

  return value;
}

function optionalNumber(body: JsonObject, key: string, errors: string[]) {
  const value = body[key];

  if (value === undefined || value === null) {
    return undefined;
  }

  if (typeof value !== "number" || !Number.isFinite(value)) {
    errors.push(`${key} deve ser um numero valido.`);
    return undefined;
  }

  return value;
}

export function validateTelemetryPayload(payload: unknown): TelemetryValidationResult {
  if (!isObject(payload)) {
    return {
      ok: false,
      errors: ["O corpo da requisicao deve ser um objeto JSON."],
    };
  }

  const errors: string[] = [];
  const data: TelemetryCreateInput = {
    sessionId: optionalString(payload, "sessionId", errors),
    robotId: optionalString(payload, "robotId", errors),
    sequence: optionalNumber(payload, "sequence", errors),
    batteryLevel: optionalNumber(payload, "batteryLevel", errors),
    positionX: optionalNumber(payload, "positionX", errors),
    positionY: optionalNumber(payload, "positionY", errors),
    headingDegrees: optionalNumber(payload, "headingDegrees", errors),
    linearVelocity: optionalNumber(payload, "linearVelocity", errors),
    angularVelocity: optionalNumber(payload, "angularVelocity", errors),
    payload: JSON.stringify(payload),
  };

  if (errors.length > 0) {
    return { ok: false, errors };
  }

  return { ok: true, data };
}
