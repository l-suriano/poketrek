// Recibe la telemetria anonima del juego: { d: aparato, v: version, ev: [{ t, k, n, x }] }.
// No hay forma de leer desde aqui; los datos se consultan con `wrangler d1 execute`.
const MAX_BODY = 32 * 1024;
const MAX_EVENTS = 60;
const clip = (s, n) => (s == null ? null : String(s).slice(0, n));

export async function onRequestPost({ request, env }) {
  const len = Number(request.headers.get('content-length') || 0);
  if (len > MAX_BODY) return new Response('too big', { status: 413 });
  let body;
  try { body = await request.json(); } catch (e) { return new Response('bad json', { status: 400 }); }
  const device = clip(body && body.d, 40);
  const events = Array.isArray(body && body.ev) ? body.ev.slice(0, MAX_EVENTS) : [];
  if (!device || !/^[a-z0-9-]{8,40}$/i.test(device) || !events.length) return new Response(null, { status: 204 });
  const stmt = env.DB.prepare('INSERT INTO events (device, version, ts, kind, name, data) VALUES (?, ?, ?, ?, ?, ?)');
  const rows = events.map(e => stmt.bind(device, clip(body.v, 20), Number(e.t) || null, clip(e.k, 20) || 'unknown',
    clip(e.n, 80), e.x == null ? null : clip(JSON.stringify(e.x), 800)));
  await env.DB.batch(rows);
  return new Response(null, { status: 204 });
}
