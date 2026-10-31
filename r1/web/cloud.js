'use strict';
// Respaldo en la nube (opcional): la partida se sube a /api/save con un codigo
// al azar que solo conoce el jugador. Con ese codigo se recupera en otro R1 o
// despues de borrar la creation. Se activa en Ajustes > Respaldo.

const CLOUD_CHARS = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';  // sin I, O, 0 ni 1
const Cloud = {
  timer: 0,
  last: 0,         // ultima subida buena (ms)
  lastData: '',    // lo ultimo que se subio: si no cambio, no se vuelve a subir
  failed: false,
  on() { return !!prefs.cloudCode && !prefs.cloudOff && location.protocol === 'https:'; },
  newCode() {
    let c = '';
    for (const b of crypto.getRandomValues(new Uint8Array(10))) c += CLOUD_CHARS[b % 32];
    return c;
  },
  pretty(c) { return c ? c.slice(0, 5) + '-' + c.slice(5) : ''; },
  async post(body) {
    const r = await fetch('/api/save', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body), keepalive: true });
    const j = await r.json().catch(() => ({}));
    if (!r.ok) throw new Error(j.error || String(r.status));
    return j;
  },
  // sube ya si esta activo y algo cambio; true si quedo respaldado
  async upload(force) {
    clearTimeout(this.timer);
    this.timer = 0;
    if (!this.on() || !Engine.call('pt_started')) return false;
    const save = Engine.call('pt_save_b64');
    if (!force && save === this.lastData) return true;
    try {
      await this.post({ op: 'put', code: prefs.cloudCode, save, profile: JSON.stringify(profile), v: TELE_VERSION });
      this.lastData = save;
      this.last = Date.now();
      this.failed = false;
      prefs.cloudAt = new Date().toISOString();
      savePrefs();
      return true;
    } catch (e) {
      this.failed = true;  // sin red: se intenta en el siguiente guardado
      return false;
    }
  },
  // despues de cada guardado: sube como mucho cada 2 minutos
  soon() {
    if (!this.on() || this.timer) return;
    const wait = Math.max(5000, 120000 - (Date.now() - this.last));
    this.timer = setTimeout(() => this.upload(), wait);
  },
  download(code) { return this.post({ op: 'get', code }); },
};

// ------------------------------------------------------------- pantallas ---

function cloudScreen() {
  const build = () => {
    if (!prefs.cloudCode || prefs.cloudOff) {
      return [
        { label: L('cloudTurnOn'), action: () => ask(L('cloudExplain'), [L('cloudTurnOn'), L('back')], k => {
          if (k !== 0) return;
          if (!prefs.cloudCode) prefs.cloudCode = Cloud.newCode();
          prefs.cloudOff = false;
          savePrefs();
          Cloud.upload(true).then(ok => message([L('cloudCodeIs', Cloud.pretty(prefs.cloudCode)), L(ok ? 'cloudWriteDown' : 'cloudLater')],
            () => topScreen().resume && topScreen().resume()));
        }) },
        { label: L('cloudRestore'), action: () => restoreFlow() },
      ];
    }
    return [
      { label: L('cloudCode'), right: Cloud.pretty(prefs.cloudCode), action: () => message([L('cloudCodeIs', Cloud.pretty(prefs.cloudCode)), L('cloudWriteDown')]) },
      { label: L('cloudLast'), right: Cloud.failed ? L('cloudFailed') : prefs.cloudAt ? prefs.cloudAt.slice(5, 16).replace('T', ' ') : '—', disabled: true },
      { label: L('cloudNow'), action: () => Cloud.upload(true).then(ok => message(L(ok ? 'cloudDone' : 'cloudLater'), () => topScreen().resume && topScreen().resume())) },
      { label: L('cloudRestore'), action: () => restoreFlow() },
      { label: L('cloudTurnOff'), action: () => ask(L('cloudOffQ'), [L('no'), L('yes')], k => {
        if (k === 1) { prefs.cloudOff = true; savePrefs(); topScreen().resume(); }
      }) },
    ];
  };
  return new ListScreen(L('cloud'), build(), { refresh: build });
}

// pide el codigo, busca el respaldo y, si el jugador confirma, lo carga y reinicia
function restoreFlow() {
  push(new KeyboardScreen(L('cloudEnter'), '', code => {
    message(L('cloudSearching'));
    const waiting = topScreen();
    Cloud.download(code).then(d => {
      if (topScreen() === waiting) pop();
      const when = (d.updated || '').slice(0, 16);
      const warn = Engine.call('pt_started') ? ' ' + L('cloudOverwrite') : '';
      ask(L('cloudFound', when) + warn, [L('cloudRestoreIt'), L('back')], k => {
        if (k !== 0) return;
        if (!Engine.call('pt_load_b64', d.save)) { message(L('cloudBad')); return; }
        try { Object.assign(profile, JSON.parse(d.profile || '{}')); } catch (e) { /* nombres por defecto */ }
        prefs.cloudCode = code;
        prefs.cloudOff = false;
        resetting = true;  // nada mas escribe la partida mientras se reinicia
        Promise.all([store.set(SAVE_KEY, d.save), store.set(PROFILE_KEY, JSON.stringify(profile)), store.set(PREF_KEY, JSON.stringify(prefs))])
          .then(() => location.reload());
      });
    }).catch(e => {
      if (topScreen() === waiting) pop();
      message(L(e.message === 'none' ? 'cloudNone' : 'cloudNoNet'));
    });
  }, { max: 10, keys: CLOUD_CHARS, group: 5 }));
}
