# Lee la telemetria anonima del R1 (base D1 "ptk-telemetry" en Cloudflare).
#   .\r1\tools\telemetry.ps1 errores       # ultimos errores
#   .\r1\tools\telemetry.ps1 pantallas     # pantallas mas abiertas
#   .\r1\tools\telemetry.ps1 combates      # combates por tipo y resultado
#   .\r1\tools\telemetry.ps1 retos         # combates de los retos de ruta por reto y resultado
#   .\r1\tools\telemetry.ps1 acciones      # compras, objetos, MTs, curas... (accion y resultado)
#   .\r1\tools\telemetry.ps1 caminata      # pasos por dia y aparato
#   .\r1\tools\telemetry.ps1 sesiones      # visitas por dia: cuantas, duracion media y pasos
#   .\r1\tools\telemetry.ps1 progreso      # donde va cada partida (ultima foto de cada aparato)
#   .\r1\tools\telemetry.ps1 hoy           # resumen de hoy por tipo de evento
#   .\r1\tools\telemetry.ps1 aparato d1a2b3c4   # lo ultimo de un aparato (su ID esta en Ajustes > ID anonimo)
#   .\r1\tools\telemetry.ps1 sql "SELECT ..."
param([string]$que = 'hoy', [string]$sql = '')

$consultas = @{
  errores   = "SELECT received, device, name, data FROM events WHERE kind = 'error' ORDER BY id DESC LIMIT 30"
  pantallas = "SELECT name, COUNT(*) AS veces FROM events WHERE kind = 'screen' GROUP BY name ORDER BY veces DESC LIMIT 30"
  combates  = "SELECT name AS tipo, json_extract(data, '$.out') AS resultado, COUNT(*) AS veces, ROUND(AVG(json_extract(data, '$.turns')), 1) AS turnos FROM events WHERE kind = 'battle' GROUP BY tipo, resultado ORDER BY tipo"
  retos     = "SELECT json_extract(data, '$.route') AS reto, json_extract(data, '$.out') AS resultado, COUNT(*) AS veces, SUM(json_extract(data, '$.cleared') = 1) AS superados FROM events WHERE kind = 'battle' AND json_extract(data, '$.route') IS NOT NULL GROUP BY reto, resultado ORDER BY reto"
  acciones  = "SELECT name AS accion, json_extract(data, '$.r') AS resultado, COUNT(*) AS veces FROM events WHERE kind = 'act' GROUP BY accion, resultado ORDER BY veces DESC LIMIT 40"
  caminata  = "SELECT date(received) AS dia, device, SUM(json_extract(data, '$.steps')) AS pasos, SUM(json_extract(data, '$.out')) AS afuera, MAX(json_extract(data, '$.km')) AS km FROM events WHERE kind = 'walk' GROUP BY dia, device ORDER BY dia DESC LIMIT 30"
  sesiones  = "SELECT date(received) AS dia, COUNT(*) AS visitas, COUNT(DISTINCT device) AS aparatos, ROUND(AVG(json_extract(data, '$.secs')) / 60.0, 1) AS minutos, SUM(json_extract(data, '$.steps')) AS pasos FROM events WHERE kind = 'session' GROUP BY dia ORDER BY dia DESC LIMIT 30"
  progreso  = "SELECT device, received, data FROM events WHERE id IN (SELECT MAX(id) FROM events WHERE kind = 'state' GROUP BY device) ORDER BY received DESC"
  hoy       = "SELECT kind, COUNT(*) AS eventos, COUNT(DISTINCT device) AS aparatos FROM events WHERE received >= date('now') GROUP BY kind ORDER BY eventos DESC"
}
if ($que -eq 'sql') { $q = $sql }
elseif ($que -eq 'aparato') {
  if ($sql -notmatch '^[a-z0-9]{8,40}$') { Write-Output 'Uso: telemetry.ps1 aparato <ID de Ajustes > ID anonimo>'; exit 1 }
  $q = "SELECT received, kind, name, data FROM events WHERE device = '$sql' ORDER BY id DESC LIMIT 80"
}
else { $q = $consultas[$que] }
if (-not $q) { Write-Output "Opciones: $($consultas.Keys -join ', '), aparato, sql"; exit 1 }
Set-Location (Join-Path $PSScriptRoot '..')
npx.cmd --yes wrangler d1 execute ptk-telemetry --remote --json --command $q
