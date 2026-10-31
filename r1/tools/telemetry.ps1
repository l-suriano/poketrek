# Lee la telemetria anonima del R1 (base D1 "ptk-telemetry" en Cloudflare).
#   .\r1\tools\telemetry.ps1 errores     # ultimos errores
#   .\r1\tools\telemetry.ps1 pantallas   # pantallas mas abiertas
#   .\r1\tools\telemetry.ps1 combates    # combates por tipo y resultado
#   .\r1\tools\telemetry.ps1 hoy         # resumen de hoy por tipo de evento
#   .\r1\tools\telemetry.ps1 sql "SELECT ..."
param([string]$que = 'hoy', [string]$sql = '')

$consultas = @{
  errores   = "SELECT received, device, name, data FROM events WHERE kind = 'error' ORDER BY id DESC LIMIT 30"
  pantallas = "SELECT name, COUNT(*) AS veces FROM events WHERE kind = 'screen' GROUP BY name ORDER BY veces DESC LIMIT 30"
  combates  = "SELECT name AS tipo, json_extract(data, '$.out') AS resultado, COUNT(*) AS veces, ROUND(AVG(json_extract(data, '$.turns')), 1) AS turnos FROM events WHERE kind = 'battle' GROUP BY tipo, resultado ORDER BY tipo"
  hoy       = "SELECT kind, COUNT(*) AS eventos, COUNT(DISTINCT device) AS aparatos FROM events WHERE received >= date('now') GROUP BY kind ORDER BY eventos DESC"
}
if ($que -eq 'sql') { $q = $sql } else { $q = $consultas[$que] }
if (-not $q) { Write-Output "Opciones: $($consultas.Keys -join ', '), sql"; exit 1 }
Set-Location (Join-Path $PSScriptRoot '..')
npx.cmd --yes wrangler d1 execute ptk-telemetry --remote --json --command $q
