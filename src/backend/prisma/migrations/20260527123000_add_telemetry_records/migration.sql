-- CreateTable
CREATE TABLE "telemetry_records" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "session_id" TEXT,
    "robot_id" TEXT,
    "sequence" INTEGER,
    "battery_level" REAL,
    "position_x" REAL,
    "position_y" REAL,
    "heading_degrees" REAL,
    "linear_velocity" REAL,
    "angular_velocity" REAL,
    "payload" TEXT,
    "received_at" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- CreateIndex
CREATE INDEX "telemetry_records_session_id_idx" ON "telemetry_records"("session_id");

-- CreateIndex
CREATE INDEX "telemetry_records_robot_id_idx" ON "telemetry_records"("robot_id");

-- CreateIndex
CREATE INDEX "telemetry_records_received_at_idx" ON "telemetry_records"("received_at");
