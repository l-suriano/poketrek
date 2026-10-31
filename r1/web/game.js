'use strict';
// Arranque de PokeTrek en el Rabbit R1.

async function boot() {
  fit();
  rect(0, 0, W, H, '#101420');
  requestAnimationFrame(loop);
  push({ draw() { ctx.setTransform(scale, 0, 0, scale, 0, 0); rect(0, 0, W, H, '#101420'); text('PokéTrek', W / 2, 120, '#d8dcf0', 'center', 16); text('...', W / 2, 150, '#6a7090', 'center'); } });

  if ('serviceWorker' in navigator && location.protocol === 'https:') {
    navigator.serviceWorker.register('sw.js').catch(() => {});
  }
  try { await document.fonts.load('8px PS2P'); } catch (e) { /* sigue con la fuente del sistema */ }
  try { SPR = await (await fetch('assets/sprites.json')).json(); } catch (e) { SPR = {}; }
  try { Object.assign(prefs, JSON.parse(await store.get(PREF_KEY) || '{}')); } catch (e) { /* por defecto */ }

  await Engine.load();
  Engine.call('pt_init', LANG_EN, nowSec(), (Math.random() * 0xFFFFFFFF) >>> 0);
  const saved = await store.get(SAVE_KEY);
  const loaded = saved && Engine.call('pt_load_b64', saved);
  startSensors();

  stack.length = 0;
  if (loaded) {
    lang = JSON.parse(Engine.call('pt_state')).lang === LANG_ES ? LANG_ES : LANG_EN;
    refresh();
    push(new HomeScreen());
  } else {
    refresh();
    push(langScreen(() => push(new StarterScreen())));
  }
}

// cada 5 s: pasos contados con el acelerometro, incienso y guardado
window.onSlowTick = () => {
  if (!Engine.call || !S || !stack.length) return;
  if (!Engine.call('pt_started')) return;
  if (steps.pending) {
    const n = steps.pending;
    steps.pending = 0;
    Engine.json('pt_walk', n, S.outside ? 1 : 0);
    saveSoon();
  }
  const t = topScreen();
  // el incienso solo saca Pokemon en pantallas tranquilas (no en combate ni lanzando)
  if (S.incense && (t instanceof HomeScreen || t instanceof ListScreen)) {
    const e = Engine.json('pt_incense_tick');
    if (e.kind === 'wild') { saveSoon(); encounterBg = 'grass'; push(new EncounterScreen(e)); }
  }
  if (t && t.resume && t instanceof HomeScreen) t.resume();
};

document.addEventListener('visibilitychange', () => { if (document.hidden) saveNow(); });

boot();
