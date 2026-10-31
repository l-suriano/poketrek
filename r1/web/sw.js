// Service worker: guarda todo el juego la primera vez para que el R1 lo abra
// despues sin internet. Al cambiar VERSION se baja de nuevo.
const VERSION = 'poketrek-r1-v8';
const CORE = ['./', 'index.html', 'text.js', 'ui.js', 'gfx.js', 'fx.js', 'screens.js', 'battle.js', 'intro.js', 'game.js', 'poketrek.js',
  'manifest.json', 'assets/sprites.json', 'assets/PressStart2P-Regular.ttf'];

const TRAINERS = ['youngster', 'bugcatcher', 'lass', 'hiker', 'fisherman', 'swimmerm', 'birdkeeper', 'supernerd',
  'teamrocketgruntm', 'engineer', 'psychic', 'blackbelt', 'channeler', 'gentleman', 'beauty', 'acetrainer',
  'brock', 'misty', 'ltsurge', 'erika', 'koga', 'sabrina', 'blaine', 'giovanni',
  'lorelei', 'bruno', 'agatha', 'lance', 'blue', 'blue-champion', 'oak', 'red'];

self.addEventListener('install', e => {
  e.waitUntil((async () => {
    const cache = await caches.open(VERSION);
    await cache.addAll(CORE);
    const spr = await (await fetch('assets/sprites.json')).json();
    await cache.addAll(Object.keys(spr).map(sp => `assets/sprites/${sp}.png`));
    await cache.addAll(TRAINERS.map(k => `assets/trainers/${k}.png`));
    self.skipWaiting();
  })());
});

self.addEventListener('activate', e => {
  e.waitUntil((async () => {
    for (const k of await caches.keys()) if (k !== VERSION) await caches.delete(k);
    self.clients.claim();
  })());
});

// primero la cache; si no esta, la red (y se guarda)
self.addEventListener('fetch', e => {
  if (e.request.method !== 'GET') return;
  e.respondWith((async () => {
    const hit = await caches.match(e.request, { ignoreSearch: true });
    if (hit) return hit;
    const res = await fetch(e.request);
    if (res.ok && new URL(e.request.url).origin === location.origin) {
      const cache = await caches.open(VERSION);
      cache.put(e.request, res.clone());
    }
    return res;
  })());
});
