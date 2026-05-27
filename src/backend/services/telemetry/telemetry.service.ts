import { prisma } from "@/lib/prisma";
import type { TelemetryCreateInput } from "./telemetry.types";

export async function createTelemetryRecord(input: TelemetryCreateInput) {
  return prisma.telemetryRecord.create({
    data: {
      sessionId: input.sessionId,
      robotId: input.robotId,
      sequence: input.sequence,
      batteryLevel: input.batteryLevel,
      positionX: input.positionX,
      positionY: input.positionY,
      headingDegrees: input.headingDegrees,
      linearVelocity: input.linearVelocity,
      angularVelocity: input.angularVelocity,
      payload: input.payload,
    },
  });
}

export async function listTelemetryRecords(limit = 50) {
  const take = Math.min(Math.max(limit, 1), 100);

  return prisma.telemetryRecord.findMany({
    orderBy: {
      receivedAt: "desc",
    },
    take,
  });
}
