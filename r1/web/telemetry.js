'use strict';
// Telemetria anonima: errores y como se usa el juego (pantallas, combates,
// capturas). Va a /api/t (Pages Function + D1 en la cuenta del duenno). Sin
// nombres ni datos personales: solo un identificador al azar del aparato.
// Se apaga en Ajustes > Enviar datos anonimos.

const TELE_VERSION = 'r1-v17';
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
