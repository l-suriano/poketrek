// Respaldo de partidas en la nube (opcional: lo activa el jugador en Ajustes).
//   { op: 'put', code, save, profile, v } guarda o reemplaza la copia del codigo
//   { op: 'get', code }                   devuelve { save, profile, updated }
// El codigo es la llave: 10 caracteres al azar (sin I, O, 0 ni 1) que genera el
// juego y solo conoce el jugador. Solo POST: el codigo nunca va en la URL.
const MAX_BODY = 96 * 1024;
const MAX_SAVE = 80 * 1024;
const CODE = /^[A-HJ-NP-Z2-9]{10}$/;
const json = (o, status = 200) => new Response(JSON.stringify(o), { status, headers: { 'Content-Type': 'application/json' } });

export async function onRequestPost({ request, env }) {
  const text = await request.text();
  if (text.length > MAX_BODY) return json({ error: 'big' }, 413);
  let b;
  try { b = JSON.parse(text); } catch (e) { return json({ error: 'json' }, 400); }
  const code = String((b && b.code) || '');
  if (!CODE.test(code)) return json({ error: 'code' }, 400);

  if (b.op === 'get') {
    const row = await env.DB.prepare('SELECT save, profile, updated FROM saves WHERE code = ?').bind(code).first();
    return row ? json(row) : json({ error: 'none' }, 404);
  }
  if (b.op === 'put') {
    const save = String(b.save || '');
    if (!save || save.length > MAX_SAVE || !/^[A-Za-z0-9+/=]+$/.test(save)) return json({ error: 'save' }, 400);
    const profile = b.profile == null ? null : String(b.profile).slice(0, 400);
    await env.DB.prepare(
      "INSERT INTO saves (code, save, profile, version, updated) VALUES (?, ?, ?, ?, datetime('now')) " +
      'ON CONFLICT(code) DO UPDATE SET save = excluded.save, profile = excluded.profile, ' +
      'version = excluded.version, updated = excluded.updated',
    ).bind(code, save, profile, String(b.v || '').slice(0, 20)).run();
    return json({ ok: true });
  }
  return json({ error: 'op' }, 400);
}
