'use strict';
// Telemetria anonima: errores y como se usa el juego (pantallas, combates,
// capturas). Va a /api/t (Pages Function + D1 en la cuenta del duenno). Sin
// nombres ni datos personales: solo un identificador al azar del aparato.
// Se apaga en Ajustes > Enviar datos anonimos.

const TELE_VERSION = 'r1-v19';
const TELE_KEY = 'poketrek_device';
const Tele = {
  queue: [],
  device: null,
  started: Date.now(),
  init() {
    try { this.device = localStorage.getItem(TELE_KEY); } catch (e) { /* sin almacenamiento */ }
    if (!this.device) {
      this.device = 'd' + Math.random().toString(36).slice(2, 10) + Date.now().toString(36);
      try { localStorage.setItem(TELE_KEY, this.device); } catch (e) { /* nada */ }
    }
    setInterval(() => this.flush(), 60000);
    document.addEventListener('visibilitychange', () => { if (document.hidden) this.flush(true); });
  },
  on() { return prefs.telemetry !== false && location.protocol === 'https:'; },
  track(k, n, x) {
    if (!this.on()) return;
    this.queue.push({ t: Date.now(), k, n: n == null ? null : String(n).slice(0, 80), x });
    if (this.queue.length >= 40) this.flush();
  },
  flush(leaving) {
    if (!this.on() || !this.queue.length) return;
    const body = JSON.stringify({ d: this.device, v: TELE_VERSION, ev: this.queue.splice(0, 60) });
    try {
      if (leaving && navigator.sendBeacon) navigator.sendBeacon('/api/t', new Blob([body], { type: 'application/json' }));
      else fetch('/api/t', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body, keepalive: true }).catch(() => {});
    } catch (e) { /* sin red: se pierde, no pasa nada */ }
  },
};

// pasos: se juntan y se mandan como mucho cada 2 minutos (no uno por cada 5 s)
const TeleWalk = {
  steps: 0, outside: 0, since: Date.now(), session: Date.now(), sessionSteps: 0,
  add(n, outside) {
    this.steps += n;
    this.sessionSteps += n;
    if (outside) this.outside += n;
    if (Date.now() - this.since > 120000) this.flush();
  },
  flush() {
    if (this.steps) Tele.track('walk', null, { steps: this.steps, out: this.outside, km: S ? Math.floor(S.meters / 100) / 10 : null });
    this.steps = this.outside = 0;
    this.since = Date.now();
  },
  // al salir de la app: duracion de la visita, pasos y donde va la partida
  end() {
    this.flush();
    Tele.track('session', null, { secs: Math.round((Date.now() - this.session) / 1000), steps: this.sessionSteps });
    teleState('leave');
  },
  start() { this.session = Date.now(); this.sessionSteps = 0; },
};

// donde va la partida: medallas, km, Pokedex, equipo, retos superados
function teleState(why) {
  try {
    if (!Engine.call('pt_started')) return;
    const s = Engine.json('pt_state');
    const ch = Engine.json('pt_challenges').list;
    Tele.track('state', why, {
      badges: popcount(s.badges), champ: s.champion, km: Math.floor(s.meters / 100) / 10, caught: s.caught, money: s.money,
      party: s.party.map(m => m.sp + ':' + m.lv), routes: ch.filter(c => c.cleared).length, box: s.boxCount, ee: s.ee,
    });
  } catch (e) { /* sin estado todavia */ }
}

// nombre corto de una pantalla para el registro de uso
function screenName(s) {
  if (!s) return '?';
  const n = s.constructor ? s.constructor.name : '?';
  return n === 'ListScreen' || n === 'Object' ? (s.title || n) : n;
}

// aviso de una vez: que se manda y donde se apaga
function telemetryNotice() {
  if (prefs.telemetryNotice || location.protocol !== 'https:') return;
  prefs.telemetryNotice = true;
  savePrefs();
  message(L('teleNotice'));
}
