'use strict';
// Arranque de PokeTrek en el Rabbit R1.

let pendingReload = false;

async function boot() {
  fit();
  rect(0, 0, W, H, '#101420');
  requestAnimationFrame(loop);
  push({ draw() { ctx.setTransform(scale, 0, 0, scale, 0, 0); rect(0, 0, W, H, '#101420'); text('PokéTrek', W / 2, 120, '#d8dcf0', 'center', 16); text('...', W / 2, 150, '#6a7090', 'center'); } });

  if ('serviceWorker' in navigator && location.protocol === 'https:') {
    // si llega una version nueva, se guarda la partida y se recarga sola
    const hadController = !!navigator.serviceWorker.controller;
    navigator.serviceWorker.addEventListener('controllerchange', () => {
      if (hadController) pendingReload = true;  // se recarga en el inicio, nunca a mitad de un combate
    });
    navigator.serviceWorker.register('sw.js').then(r => r.update()).catch(() => {});
  }
  try { await document.fonts.load('8px PS2P'); } catch (e) { /* sigue con la fuente del sistema */ }
  try { SPR = await (await fetch('assets/sprites.json')).json(); } catch (e) { SPR = {}; }
  try { Object.assign(prefs, JSON.parse(await store.get(PREF_KEY) || '{}')); } catch (e) { /* por defecto */ }
  await loadProfile();

  Tele.init();
  await Engine.load();
  Engine.call('pt_init', LANG_EN, nowSec(), (Math.random() * 0xFFFFFFFF) >>> 0);
  const saved = await store.get(SAVE_KEY);
  const loaded = saved && Engine.call('pt_load_b64', saved);
  startSensors();
  Tele.track('boot', lang === LANG_ES ? 'es' : 'en', { save: !!loaded, w: innerWidth, h: innerHeight, r1: !!window.creationStorage });

  stack.length = 0;
  if (loaded) {
    lang = JSON.parse(Engine.call('pt_state')).lang === LANG_ES ? LANG_ES : LANG_EN;
    refresh();
    push(new HomeScreen());
    teleState('boot');
    telemetryNotice();
  } else {
    refresh();
    push(langScreen(() => push(new IntroScreen(() => push(new StarterScreen())))));
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
    TeleWalk.add(n, S.outside);
    saveSoon();
  }
  const t = topScreen();
  if (t instanceof HomeScreen && S.outside) { refresh(); maybeAutoEvent(); }
  // el incienso solo saca Pokemon en pantallas tranquilas (no en combate ni lanzando)
  if (pendingReload && t instanceof HomeScreen) { saveNow(); location.reload(); return; }
  // el incienso solo saca Pokemon en el inicio (no en dialogos, combates ni menus)
  if (S.incense && t instanceof HomeScreen) {
    const e = Engine.json('pt_incense_tick');
    if (e.kind === 'wild') { saveSoon(); encounterBg = 'grass'; push(new EncounterScreen(e)); }
  }
  if (t && t.resume && t instanceof HomeScreen) t.resume();
};

document.addEventListener('visibilitychange', () => {
  if (document.hidden) { saveNow(); Cloud.upload(); TeleWalk.end(); Tele.flush(true); }
  else TeleWalk.start();
});

boot();
