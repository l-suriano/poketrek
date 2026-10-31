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

-- Respaldo opcional de partidas: una fila por codigo (lo genera el juego al azar).
CREATE TABLE IF NOT EXISTS saves (
  code TEXT PRIMARY KEY,     -- 10 caracteres al azar: la llave del respaldo
  save TEXT NOT NULL,        -- partida en base64 (pt_save_b64)
  profile TEXT,              -- nombres del jugador y del rival (JSON corto)
  version TEXT,
  updated TEXT NOT NULL DEFAULT (datetime('now'))
);
