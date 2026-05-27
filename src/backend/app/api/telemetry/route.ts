import { NextResponse } from "next/server";
import {
  createTelemetryRecord,
  listTelemetryRecords,
} from "@/services/telemetry/telemetry.service";
import { validateTelemetryPayload } from "@/services/telemetry/telemetry.validator";

export const runtime = "nodejs";

export async function GET(request: Request) {
  const { searchParams } = new URL(request.url);
  const limitParam = searchParams.get("limit");
  const limit = limitParam ? Number(limitParam) : 50;

  try {
    const records = await listTelemetryRecords(Number.isFinite(limit) ? limit : 50);

    return NextResponse.json({ data: records }, { status: 200 });
  } catch {
    return NextResponse.json(
      { error: "Nao foi possivel consultar os registros de telemetria." },
      { status: 500 },
    );
  }
}

export async function POST(request: Request) {
  let body: unknown;

  try {
    body = await request.json();
  } catch {
    return NextResponse.json(
      { error: "JSON invalido no corpo da requisicao." },
      { status: 400 },
    );
  }

  const validation = validateTelemetryPayload(body);

  if (!validation.ok) {
    return NextResponse.json(
      { error: "Payload de telemetria invalido.", details: validation.errors },
      { status: 422 },
    );
  }

  try {
    const record = await createTelemetryRecord(validation.data);

    return NextResponse.json({ data: record }, { status: 201 });
  } catch {
    return NextResponse.json(
      { error: "Nao foi possivel registrar a telemetria." },
      { status: 500 },
    );
  }
}
