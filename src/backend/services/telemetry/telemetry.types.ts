export type TelemetryCreateInput = {
  sessionId?: string;
  robotId?: string;
  sequence?: number;
  batteryLevel?: number;
  positionX?: number;
  positionY?: number;
  headingDegrees?: number;
  linearVelocity?: number;
  angularVelocity?: number;
  payload: string;
};

export type TelemetryValidationResult =
  | {
      ok: true;
      data: TelemetryCreateInput;
    }
  | {
      ok: false;
      errors: string[];
    };
