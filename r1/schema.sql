-- Telemetria anonima de PokeTrek R1: un evento por fila.
CREATE TABLE IF NOT EXISTS events (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  received TEXT NOT NULL DEFAULT (datetime('now')),
  device TEXT NOT NULL,      -- identificador al azar del aparato (no es un dato personal)
  version TEXT,
  ts INTEGER,                -- hora del aparato (ms)
  kind TEXT NOT NULL,        -- error, screen, battle, catch, encounter, boot...
  name TEXT,
  data TEXT                  -- JSON corto con el detalle
);
CREATE INDEX IF NOT EXISTS events_kind ON events (kind, received);
CREATE INDEX IF NOT EXISTS events_device ON events (device, received);
