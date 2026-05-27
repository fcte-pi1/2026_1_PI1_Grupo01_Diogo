-- CreateTable
CREATE TABLE "navigation_sessions" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "user_id" TEXT,
    "started_at" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "ended_at" DATETIME,
    "created_at" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updated_at" DATETIME NOT NULL
);

-- CreateTable
CREATE TABLE "navigation_events" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "session_id" TEXT NOT NULL,
    "url" TEXT NOT NULL,
    "path" TEXT,
    "title" TEXT,
    "referrer" TEXT,
    "user_agent" TEXT,
    "metadata" TEXT,
    "occurred_at" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "navigation_events_session_id_fkey" FOREIGN KEY ("session_id") REFERENCES "navigation_sessions" ("id") ON DELETE CASCADE ON UPDATE CASCADE
);

-- CreateIndex
CREATE INDEX "navigation_sessions_user_id_idx" ON "navigation_sessions"("user_id");

-- CreateIndex
CREATE INDEX "navigation_sessions_started_at_idx" ON "navigation_sessions"("started_at");

-- CreateIndex
CREATE INDEX "navigation_events_session_id_idx" ON "navigation_events"("session_id");

-- CreateIndex
CREATE INDEX "navigation_events_occurred_at_idx" ON "navigation_events"("occurred_at");

-- CreateIndex
CREATE INDEX "navigation_events_url_idx" ON "navigation_events"("url");
