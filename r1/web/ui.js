'use strict';
// Base de la interfaz: motor (WebAssembly o servidor de desarrollo), pantalla,
// dibujo, sprites, controles del R1, guardado y la pila de pantallas.

const W = 240, H = 282;  // pantalla de una creation del R1

// ------------------------------------------------------------------ motor ---

// funciones del puente que devuelven texto (el resto devuelve numeros)
const STR_FNS = new Set(['pt_state', 'pt_species', 'pt_box', 'pt_box_page', 'pt_box_mon', 'pt_mon', 'pt_dex', 'pt_species_name', 'pt_walk', 'pt_next_event',
  'pt_routes', 'pt_search', 'pt_encounter', 'pt_incense_tick', 'pt_throw', 'pt_battle_start', 'pt_battle_view',
  'pt_battle_act', 'pt_battle_end', 'pt_pending', 'pt_use_item', 'pt_shop', 'pt_gyms', 'pt_tour_info',
  'pt_tour_finish', 'pt_type_name', 'pt_save_b64', 'pt_challenges']);

const Engine = {
  mod: null,
  dev: false,
  call(fn, ...args) {
    const str = STR_FNS.has(fn);
    if (this.mod) {
      return this.mod.ccall(fn, str ? 'string' : 'number', args.map(a => typeof a === 'string' ? 'string' : 'number'), args);
    }
    // servidor de desarrollo (r1/tools/dev_server.py): mismo motor compilado en nativo
    const q = args.map(a => 'a=' + encodeURIComponent(a)).join('&');
    const x = new XMLHttpRequest();
    x.open('GET', '/api/' + fn + (q ? '?' + q : ''), false);
    x.send();
    return str ? x.responseText : Number(x.responseText || 0);
  },
  json(fn, ...args) { return JSON.parse(this.call(fn, ...args)); },
  async load() {
    try {
      await new Promise((ok, bad) => {
        const s = document.createElement('script');
        s.src = 'poketrek.js';
        s.onload = ok;
        s.onerror = bad;
        document.head.appendChild(s);
      });
      this.mod = await PokeTrekModule();
    } catch (e) {
      this.dev = true;
    }
  },
};

// hora local en segundos (el motor cuenta los dias a medianoche local)
function nowSec() { return Math.floor(Date.now() / 1000) - new Date().getTimezoneOffset() * 60; }
function tick() { Engine.call('pt_tick', nowSec()); }

// --------------------------------------------------------------- guardado ---

const SAVE_KEY = 'poketrek_save';
const PREF_KEY = 'poketrek_prefs';
const store = {
  async get(k) {
    const cs = window.creationStorage;
    if (cs && cs.plain) {
      try {
        const v = await cs.plain.getItem(k);
        return v == null ? null : atob(v);
      } catch (e) { /* cae a localStorage */ }
    }
    try { return localStorage.getItem(k); } catch (e) { return null; }
  },
  async set(k, v) {
    const cs = window.creationStorage;
    if (cs && cs.plain) {
      try { await cs.plain.setItem(k, btoa(v)); return; } catch (e) { /* cae a localStorage */ }
    }
    try { localStorage.setItem(k, v); } catch (e) { /* sin almacenamiento */ }
  },
  async remove(k) {
    const cs = window.creationStorage;
    if (cs && cs.plain) { try { await cs.plain.removeItem(k); } catch (e) { /* nada */ } }
    try { localStorage.removeItem(k); } catch (e) { /* nada */ }
  },
};

let saveTimer = 0;
function saveSoon() {
  clearTimeout(saveTimer);
  saveTimer = setTimeout(saveNow, 800);
}
let resetting = false;
function saveNow() {
  if (resetting) return;
  if (!Engine.call('pt_started')) return;
  store.set(SAVE_KEY, Engine.call('pt_save_b64'));
  if (typeof Cloud !== 'undefined') Cloud.soon();
}

const prefs = { invertX: false, invertY: false };
function savePrefs() { store.set(PREF_KEY, JSON.stringify(prefs)); }

// ---------------------------------------------------------------- pantalla --

const canvas = document.getElementById('screen');
const ctx = canvas.getContext('2d');
let scale = 1;

function fit() {
  const dpr = window.devicePixelRatio || 1;
  const s = Math.min(window.innerWidth / W, window.innerHeight / H);
  canvas.style.width = Math.floor(W * s) + 'px';
  canvas.style.height = Math.floor(H * s) + 'px';
  scale = Math.max(1, Math.round(s * dpr));
  canvas.width = W * scale;
  canvas.height = H * scale;
  ctx.imageSmoothingEnabled = false;
}
window.addEventListener('resize', fit);

// colores: tema de dia y de noche (cambia con la hora local)
const THEMES = {
  day: { bg: '#98c8e0', pat: '#a8d4ea', ink: '#383840', dim: '#808890', panel: '#f8f8f8', line: '#405060', sel: '#c03028', track: '#c8d0d0', accent: '#3880e0' },
  night: { bg: '#283050', pat: '#303a5c', ink: '#383840', dim: '#808890', panel: '#f8f8f8', line: '#405060', sel: '#c03028', track: '#c8d0d0', accent: '#3880e0' },
};
let theme = THEMES.day;

const TYPE_COLORS = ['#a8a878', '#c03028', '#a890f0', '#a040a0', '#e0c068', '#b8a038', '#a8b820', '#705898',
  '#f08030', '#6890f0', '#78c850', '#f8d030', '#f85888', '#98d8d8', '#7038f8'];
const typeNames = {};
function typeName(t) {
  if (t === 255) return '';
  const k = lang + ':' + t;
  if (!(k in typeNames)) typeNames[k] = JSON.parse(Engine.call('pt_type_name', t));
  return typeNames[k];
}

const FONT = 'PS2P';
function setFont(px) { ctx.font = `${px}px ${FONT}`; }

// texto en la cuadricula de 8 px de Press Start 2P
function text(s, x, y, color = theme.ink, align = 'left', px = 8) {
  setFont(px);
  ctx.fillStyle = color;
  ctx.textAlign = align;
  ctx.textBaseline = 'top';
  ctx.fillText(s, x, y);
}

// parte un texto en lineas de `max` caracteres
function wrap(s, max) {
  const out = [];
  for (const para of String(s).split('\n')) {
    let line = '';
    for (const w of para.split(' ')) {
      if ((line + (line ? ' ' : '') + w).length > max) { if (line) out.push(line); line = w; }
      else line += (line ? ' ' : '') + w;
    }
    out.push(line);
  }
  return out;
}

function rect(x, y, w, h, color) { ctx.fillStyle = color; ctx.fillRect(x, y, w, h); }
function frame(x, y, w, h, fill = theme.panel, line = theme.line) {
  rect(x, y, w, h, line);
  rect(x + 1, y + 1, w - 2, h - 2, fill);
}

function hpColor(f) { return f > 0.5 ? '#58b868' : f > 0.2 ? '#e8a23c' : '#e8503a'; }
function bar(x, y, w, h, f, color) {
  rect(x, y, w, h, theme.track);
  rect(x, y, Math.round(w * Math.max(0, Math.min(1, f))), h, color || hpColor(f));
}

function typeChip(t, x, y) {
  const n = typeName(t).toUpperCase();
  setFont(8);
  const w = Math.max(24, ctx.measureText(n).width + 6);
  rect(x, y, w, 11, TYPE_COLORS[t] || '#888');
  text(n, x + w / 2, y + 2, '#fff', 'center');
  return w;
}

const STATUS_TXT = [['', 'PAR', 'DOR', 'CON', 'QUE', 'ENV', 'ENV'], ['', 'PAR', 'SLP', 'FRZ', 'BRN', 'PSN', 'PSN']];
const STATUS_COLOR = ['', '#d8b020', '#8a8a9a', '#68b8d8', '#e86030', '#a050c0', '#a050c0'];
function statusChip(st, x, y) {
  if (!st) return;
  rect(x, y, 26, 10, STATUS_COLOR[st]);
  text(STATUS_TXT[lang === LANG_ES ? 0 : 1][st], x + 13, y + 1, '#fff', 'center');
}

// ---------------------------------------------------------------- sprites ---

let SPR = {};
const sprImg = {};
const sprBody = {};  // caja visible del primer cuadro de Idle: { top, bottom, h } por lado

function spriteImage(sp) {
  if (!SPR[sp]) return null;
  if (!sprImg[sp]) {
    const img = new Image();
    img.onload = () => measureBody(sp, img);
    img.src = `assets/sprites/${sp}.png`;
    sprImg[sp] = img;
  }
  return sprImg[sp].complete && sprImg[sp].naturalWidth ? sprImg[sp] : null;
}

// mide donde empieza y acaba el cuerpo (los cuadros de PMD llevan mucho margen)
function measureBody(sp, img) {
  const out = {};
  for (const side of ['back', 'front']) {
    const m = SPR[sp]['Idle_' + side];
    if (!m) continue;
    const c = document.createElement('canvas');
    c.width = m.w; c.height = m.h;
    const g = c.getContext('2d');
    g.drawImage(img, 0, m.y, m.w, m.h, 0, 0, m.w, m.h);
    const d = g.getImageData(0, 0, m.w, m.h).data;
    let top = m.h, bottom = 0;
    for (let y = 0; y < m.h; y++)
      for (let x = 0; x < m.w; x++)
        if (d[(y * m.w + x) * 4 + 3] > 40) { if (y < top) top = y; if (y > bottom) bottom = y; }
    out[side] = top <= bottom ? { top, bottom, h: bottom - top + 1 } : { top: 0, bottom: m.h - 1, h: m.h };
  }
  sprBody[sp] = out;
}

// tamano en pantalla segun la altura real (raiz 0,6 para que los enormes quepan)
function monScale(sp, hd, side, k = 34) {
  const body = sprBody[sp] && sprBody[sp][side];
  if (!body) return 1;
  const target = Math.max(14, Math.min(92, k * Math.pow(Math.max(1, hd) / 10, 0.6)));
  return target / body.h;
}

// dibuja un Pokemon con los pies en (x, y). anim: Idle | Attack | Hurt
function drawMon(sp, side, x, y, t, opts = {}) {
  const img = spriteImage(sp);
  const anim = opts.anim || 'Idle';
  const m = SPR[sp] && (SPR[sp][anim + '_' + side] || SPR[sp]['Idle_' + side]);
  if (!img || !m) {
    // sin sprite: una silueta con el numero
    rect(x - 12, y - 24, 24, 24, theme.dim);
    text(String(sp), x, y - 16, theme.bg, 'center');
    return;
  }
  const total = m.d.reduce((a, b) => a + b, 0);
  let f = Math.floor(t * 60 / 1000) % total, fi = 0;
  while (f >= m.d[fi]) { f -= m.d[fi]; fi++; }
  if (opts.frame !== undefined) fi = opts.frame % m.d.length;
  const s = opts.scale || 1;
  const body = (sprBody[sp] && sprBody[sp][side]) || { bottom: m.h - 1 };
  const dw = m.w * s, dh = m.h * s;
  const dx = Math.round(x - dw / 2), dy = Math.round(y - (body.bottom + 1) * s);
  // el destello blanco se pinta en un lienzo aparte: sobre la pantalla pintaria
  // un cuadrado encima del fondo
  let src = img, sx = fi * m.w, sy = m.y;
  if (opts.flash) {
    flashCanvas.width = m.w; flashCanvas.height = m.h;
    flashCtx.drawImage(img, sx, sy, m.w, m.h, 0, 0, m.w, m.h);
    flashCtx.globalCompositeOperation = 'source-atop';
    flashCtx.globalAlpha = opts.flash;
    flashCtx.fillStyle = '#ffffff';
    flashCtx.fillRect(0, 0, m.w, m.h);
    flashCtx.globalCompositeOperation = 'source-over';
    flashCtx.globalAlpha = 1;
    src = flashCanvas; sx = 0; sy = 0;
  }
  ctx.save();
  if (opts.alpha !== undefined) ctx.globalAlpha = opts.alpha;
  if (opts.flip) { ctx.translate(dx + dw, dy); ctx.scale(-1, 1); ctx.drawImage(src, sx, sy, m.w, m.h, 0, 0, dw, dh); }
  else ctx.drawImage(src, sx, sy, m.w, m.h, dx, dy, dw, dh);
  ctx.restore();
}
const flashCanvas = document.createElement('canvas');
const flashCtx = flashCanvas.getContext('2d');

// mini icono (de frente, encajado en una caja de `box` px)
function drawIcon(sp, x, y, box, t) {
  const img = spriteImage(sp);
  const body = sprBody[sp] && sprBody[sp].front;
  if (!img || !body) { rect(x + box / 4, y + box / 4, box / 2, box / 2, theme.dim); return; }
  const s = Math.min(1.5, box / Math.max(body.h, 1));
  drawMon(sp, 'front', x + box / 2, y + box - 1, t, { scale: s });
}

// ---------------------------------------------------------------- controles -
// R1: rueda (scrollUp/scrollDown), boton lateral (sideClick = aceptar, pulsacion
// larga = volver) y pantalla tactil. En el PC: flechas, Intro, Esc y raton.

const input = { tilt: { x: 0, y: 0, z: 1, ok: false }, mouse: null, wheelUps: [] };

function dispatch(ev) {
  const s = topScreen();
  try { if (s && s.input) s.input(ev); } catch (e) { logError(e, 'input:' + (s && s.constructor ? s.constructor.name : '?') + ':' + ev.k); }
  needDraw = true;
}

window.addEventListener('scrollUp', () => { input.wheelUps.push(performance.now()); dispatch({ k: 'up', wheel: true }); });
window.addEventListener('scrollDown', () => { input.wheelUps.push(performance.now()); dispatch({ k: 'down', wheel: true }); });
window.addEventListener('sideClick', () => dispatch({ k: 'ok', side: true }));
let longAt = 0;
window.addEventListener('longPressStart', () => { longAt = performance.now(); dispatch({ k: 'back' }); });

window.addEventListener('keydown', e => {
  const map = { ArrowUp: 'up', ArrowDown: 'down', ArrowLeft: 'left', ArrowRight: 'right', Enter: 'ok', ' ': 'ok',
                Escape: 'back', Backspace: 'back' };
  const k = map[e.key];
  if (!k) return;
  e.preventDefault();
  if (k === 'up') input.wheelUps.push(performance.now());
  dispatch({ k, key: e.key });
});
canvas.addEventListener('wheel', e => {
  e.preventDefault();
  if (e.deltaY < 0) input.wheelUps.push(performance.now());
  dispatch({ k: e.deltaY < 0 ? 'up' : 'down', wheel: true });
}, { passive: false });

function canvasPos(e) {
  const r = canvas.getBoundingClientRect();
  return { x: (e.clientX - r.left) * W / r.width, y: (e.clientY - r.top) * H / r.height };
}
// tocar = soltar sin moverse; si el dedo se mueve mas de 6 px es arrastrar (el mapa)
let ptr = null;
canvas.addEventListener('pointerdown', e => {
  const p = canvasPos(e);
  input.mouse = p;
  ptr = { x: p.x, y: p.y, sx: p.x, sy: p.y, moved: false };
  try { canvas.setPointerCapture(e.pointerId); } catch (x) { /* nada */ }
});
canvas.addEventListener('pointermove', e => {
  const p = canvasPos(e);
  input.mouse = p;
  if (!ptr) return;
  if (!ptr.moved && Math.hypot(p.x - ptr.sx, p.y - ptr.sy) > 6) ptr.moved = true;
  if (ptr.moved) dispatch({ k: 'drag', dx: p.x - ptr.x, dy: p.y - ptr.y, x: p.x, y: p.y });
  ptr.x = p.x; ptr.y = p.y;
});
canvas.addEventListener('pointerup', () => {
  if (ptr && !ptr.moved) dispatch({ k: 'tap', x: ptr.sx, y: ptr.sy });
  ptr = null;
});
canvas.addEventListener('pointercancel', () => { ptr = null; });

// acelerometro del R1 (apuntar y contar pasos)
const steps = { count: 0, last: 0, avg: 1, pending: 0, lastFlush: 0 };
function onAccel(d) {
  input.tilt = { x: d.x, y: d.y, z: d.z, ok: true };
  // pasos: picos de la magnitud por encima de la media, con ritmo de caminar
  const m = Math.sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
  steps.avg = steps.avg * 0.95 + m * 0.05;
  const t = performance.now();
  if (m > steps.avg + 0.12 && t - steps.last > 300 && t - steps.last < 1200) steps.pending++;
  if (m > steps.avg + 0.12 && t - steps.last > 300) steps.last = t;
}
async function startSensors() {
  const s = window.creationSensors && window.creationSensors.accelerometer;
  if (!s) return;
  try {
    if (await s.isAvailable()) s.start(onAccel, { frequency: 30 });
  } catch (e) { /* sin acelerometro */ }
}

// ------------------------------------------------------ pila de pantallas --

const stack = [];
let needDraw = true;
function topScreen() { return stack[stack.length - 1]; }
function push(s) {
  stack.push(s);
  if (typeof Tele !== 'undefined') Tele.track('screen', screenName(s));
  if (s.enter) s.enter();
  needDraw = true;
}
function pop() { const s = stack.pop(); if (s && s.leave) s.leave(); const t = topScreen(); if (t && t.resume) t.resume(); needDraw = true; }
function replace(s) { stack.pop(); push(s); }
function home() { while (stack.length > 1) stack.pop(); const t = topScreen(); if (t && t.resume) t.resume(); needDraw = true; }

// boton ◀ arriba a la izquierda: en el R1 la presion larga la toma el sistema
function backButton(x = 2, y = 2) {
  rect(x, y, 18, 14, '#c03028');
  rect(x + 1, y + 1, 16, 12, '#e85840');
  ctx.fillStyle = '#f8f8f8';
  for (let i = 0; i < 4; i++) ctx.fillRect(x + 6 + i, y + 7 - i, 1, 1 + i * 2);
  ctx.fillRect(x + 10, y + 6, 3, 3);
}
function inBack(ev, x = 2, y = 2) { return ev.k === 'tap' && ev.x >= x - 8 && ev.x <= x + 28 && ev.y >= y - 8 && ev.y <= y + 24; }

// corta un texto con "…" para que quepa en `w` pixeles
function fitText(s, w) {
  s = String(s);
  setFont(8);
  if (ctx.measureText(s).width <= w) return s;
  while (s.length > 1 && ctx.measureText(s + '…').width > w) s = s.slice(0, -1);
  return s + '…';
}

// fondo de los menus: rayas diagonales suaves, como las pantallas de los juegos
function menuBg() {
  rect(0, 0, W, H, theme.bg);
  ctx.fillStyle = theme.pat;
  for (let x = -H; x < W; x += 8) {
    ctx.beginPath(); ctx.moveTo(x, H); ctx.lineTo(x + H, 0); ctx.lineTo(x + H + 3, 0); ctx.lineTo(x + 3, H); ctx.fill();
  }
}

// encabezado comun
// back: dibuja el boton ◀ (tocar el encabezado vuelve atras)
function header(title, right, back) {
  rect(0, 0, W, 18, '#304868');
  rect(0, 17, W, 1, '#6890c0');
  rect(0, 18, W, 1, '#182438');
  if (back) { backButton(); textS(title, 24, 5, '#f8f8f8', 'left', '#182438'); }
  else textS(title, 6, 5, '#f8f8f8', 'left', '#182438');
  if (right) textS(right, W - 6, 5, '#f8f8f8', 'right', '#182438');
}

// --------------------------------------------------------------- widgets ----

// lista con la rueda: items { label, right, sub, disabled, icon, action }
class ListScreen {
  constructor(title, items, opts = {}) {
    this.title = title;
    this.items = items;
    this.sel = opts.sel || 0;
    this.opts = opts;
    this.rowH = opts.rowH || 22;
    this.top = 0;
    this.items = this.withBack(items);
  }
  // las listas que se pueden cerrar llevan "◀ Volver" al final (rueda + boton)
  withBack(items) {
    if (this.opts.onBack || this.opts.noBack) return items;
    return items.concat([{ label: '◀ ' + L('back'), isBack: true, action: () => pop() }]);
  }
  get y0() { return this.opts.y0 || 26; }
  get visible() { return Math.floor((H - this.y0 - (this.opts.footer ? 18 : this.opts.descBox ? 40 : 6)) / this.rowH); }
  resume() { if (this.opts.refresh) { this.items = this.withBack(this.opts.refresh()); this.sel = Math.min(this.sel, Math.max(0, this.items.length - 1)); } }
  draw(t) {
    menuBg();
    header(this.title, typeof this.opts.right === 'function' ? this.opts.right() : this.opts.right, !this.opts.onBack);
    if (this.opts.drawTop) this.opts.drawTop(t);
    if (this.sel < this.top) this.top = this.sel;
    if (this.sel >= this.top + this.visible) this.top = this.sel - this.visible + 1;
    const rows = Math.min(this.visible, Math.max(1, this.items.length));
    roundBox(3, this.y0 - 4, W - 6, rows * this.rowH + 8, BOX.light);
    if (!this.items.length) textS(L('empty'), W / 2, this.y0 + (this.rowH - 8) / 2, INK_DIM, 'center');
    for (let i = this.top; i < Math.min(this.items.length, this.top + this.visible); i++) {
      const it = this.items[i];
      const y = this.y0 + (i - this.top) * this.rowH;
      const on = i === this.sel;
      if (on) { rect(7, y, W - 14, this.rowH - 1, '#e0ecf8'); rect(7, y + this.rowH - 1, W - 14, 1, '#b8cce0'); }
      const ty = it.sub ? y + 3 : y + (this.rowH - 8) / 2;
      if (on) cursor(9, ty, '#c03028');
      let x = 18;
      if (it.icon) { drawIcon(it.icon, 15, y - 1, this.rowH, t); x = this.rowH + 17; }
      const color = it.isBack ? INK_DIM : it.disabled ? INK_DIM : INK;
      const right = it.right !== undefined && it.right !== '' ? String(it.right) : '';
      setFont(8);
      const rw = right ? ctx.measureText(right).width + 8 : 0;
      textS(fitText(it.label, W - 11 - x - rw), x, ty, color);
      if (right) textS(right, W - 11, ty, it.disabled ? INK_DIM : (it.rightColor || INK), 'right');
      if (it.sub) text(fitText(it.sub, W - 11 - x - (it.hp !== undefined ? 60 : 0)), x, y + 13, INK_DIM);
      if (it.hp !== undefined) bar(W - 64, y + 15, 53, 3, it.hp);
    }
    if (this.items.length > this.visible) {
      const h = rows * this.rowH;
      rect(W - 7, this.y0 + h * this.top / this.items.length, 2, Math.max(6, h * this.visible / this.items.length), '#8098b0');
    }
    if (this.opts.descBox) {
      // descripcion del elegido, como la mochila del juego
      const it = this.items[this.sel];
      roundBox(3, H - 36, W - 6, 34, BOX.dark);
      if (it && it.desc) wrap(it.desc, 26).slice(0, 2).forEach((l, i) => text(l, 10, H - 29 + i * 12, '#f8f8f8'));
    }
    if (this.opts.footer) {
      const f = typeof this.opts.footer === 'function' ? this.opts.footer() : this.opts.footer;
      if (f) { roundBox(3, H - 18, W - 6, 16, BOX.light); text(f, 9, H - 13, INK); }
    }
  }
  input(ev) {
    const n = this.items.length;
    if (ev.k === 'up' || ev.k === 'left') this.sel = n ? (this.sel - 1 + n) % n : 0;
    else if (ev.k === 'down' || ev.k === 'right') this.sel = n ? (this.sel + 1) % n : 0;
    else if (ev.k === 'back') { if (this.opts.onBack) this.opts.onBack(); else pop(); }
    else if (ev.k === 'ok') this.choose(this.sel);
    else if (ev.k === 'tap') {
      if (ev.y < 18) { if (this.opts.onBack) this.opts.onBack(); else pop(); return; }
      const i = this.top + Math.floor((ev.y - this.y0) / this.rowH);
      if (i >= 0 && i < n && ev.y >= this.y0) { if (i === this.sel) this.choose(i); else this.sel = i; }
    }
  }
  choose(i) {
    const it = this.items[i];
    if (!it || it.disabled) { if (it && it.why) message(it.why); return; }
    if (it.action) it.action(it, i);
  }
}

// cuadro de dialogo de los juegos: abajo, con el triangulo que parpadea
function dialogBox(lines, t, arrow) {
  const h = 16 + lines.length * 12;
  const y = H - h - 3;
  roundBox(3, y, W - 6, h, BOX.light);
  lines.forEach((l, i) => textS(l, 11, y + 8 + i * 12));
  if (arrow && Math.floor(t / 400) % 2) {
    ctx.fillStyle = '#c03028';
    for (let i = 0; i < 4; i++) ctx.fillRect(W - 18 + i, y + h - 12 + i, 7 - i * 2, 1);
  }
  return y;
}

// mensaje: texto con salto de linea automatico; aceptar o tocar para seguir
class MessageScreen {
  constructor(lines, then, opts = {}) {
    this.pages = [];
    const all = [].concat(lines).flatMap(l => wrap(l, 26));
    for (let i = 0; i < all.length; i += 6) this.pages.push(all.slice(i, i + 6));
    this.page = 0;
    this.then = then;
    this.opts = opts;
  }
  draw(t) {
    const under = stack[stack.indexOf(this) - 1];  // la de debajo de esta (no de la de arriba)
    if (under && under !== this) under.draw(t);
    dialogBox(this.pages[this.page] || [], t, true);
  }
  input(ev) {
    if (ev.k !== 'ok' && ev.k !== 'tap' && ev.k !== 'back') return;
    if (++this.page >= this.pages.length) { pop(); if (this.then) this.then(); }
  }
}
function message(lines, then) { push(new MessageScreen(lines, then)); }

// pregunta con opciones: el texto abajo y las opciones en un recuadro a la derecha
function ask(question, options, onPick) {
  const items = options.map((o, i) => ({ label: o, action: () => { pop(); onPick(i); } }));
  const s = new ListScreen('', items, { rowH: 18, noBack: true });
  s.draw = function (t) {
    const under = stack[stack.indexOf(this) - 1];  // la de debajo de esta (no de la de arriba)
    if (under && under !== this) under.draw(t);
    const y = dialogBox(wrap(question, 26).slice(0, 4), t, false);
    setFont(8);
    const w = Math.min(W - 10, Math.max(...items.map(it => ctx.measureText(it.label).width)) + 30);
    const h = items.length * 18 + 10;
    const by = y - h - 2;
    roundBox(W - w - 4, by, w, h, BOX.light);
    items.forEach((it, i) => {
      if (i === this.sel) cursor(W - w + 4, by + 9 + i * 18, INK);
      textS(it.label, W - w + 14, by + 9 + i * 18);
    });
    this._box = { x: W - w - 4, y: by + 5 };
  };
  const baseInput = s.input.bind(s);
  s.input = function (ev) {
    if (ev.k === 'tap') {
      const b = this._box || { x: 0, y: 0 };
      const i = Math.floor((ev.y - b.y) / 18);
      if (ev.x >= b.x && i >= 0 && i < items.length) { this.sel = i; this.choose(i); }
      return;
    }
    if (ev.k === 'back') return;  // hay que elegir
    baseInput(ev);
  };
  push(s);
}

// elegir una cantidad con la rueda
function askNumber(title, max, unitPrice, onPick) {
  let n = 1;
  push({
    draw(t) {
      const under = stack[stack.indexOf(this) - 1];  // la de debajo de esta (no de la de arriba)
      if (under) under.draw(t);
      roundBox(30, 96, W - 60, 76, BOX.light);
      textS(title, W / 2, 106, INK, 'center');
      textS('×' + String(n).padStart(2, '0'), W / 2, 126, '#c03028', 'center');
      cursor(W / 2 - 40, 126, INK);
      ctx.save(); ctx.translate(W / 2 + 40, 134); ctx.scale(-1, 1); cursor(0, -8, INK); ctx.restore();
      if (unitPrice) textS(unitPrice(n), W / 2, 148, INK_DIM, 'center');
    },
    input(ev) {
      if (ev.k === 'up' || ev.k === 'right') n = n >= max ? 1 : n + 1;
      else if (ev.k === 'down' || ev.k === 'left') n = n <= 1 ? max : n - 1;
      else if (ev.k === 'ok') { pop(); onPick(n); }
      else if (ev.k === 'back') pop();
      else if (ev.k === 'tap') {
        if (ev.y < 96 || ev.y > 172) pop();
        else if (ev.x < 90) n = n <= 1 ? max : n - 1;
        else if (ev.x > 150) n = n >= max ? 1 : n + 1;
        else { pop(); onPick(n); }
      }
    },
  });
}

// ------------------------------------------------------------------ bucle ---

let lastStateTick = 0;
// un error al dibujar no puede congelar el juego: se anota y el ciclo sigue
function loop(t) {
  requestAnimationFrame(loop);
  const s = topScreen();
  try {
    if (s) {
      ctx.setTransform(scale, 0, 0, scale, 0, 0);
      ctx.globalAlpha = 1;
      ctx.globalCompositeOperation = 'source-over';
      s.draw(t);
    }
    // cada 5 s: pasos del acelerometro e incienso
    if (t - lastStateTick > 5000) {
      lastStateTick = t;
      if (window.onSlowTick) window.onSlowTick();
    }
  } catch (e) {
    logError(e, 'draw:' + (s && s.constructor ? s.constructor.name : '?'));
  }
}

// --------------------------------------------------- registro de errores ---
// Se guardan los ultimos errores en el aparato (Ajustes > Registro de errores).
const ERRLOG_KEY = 'poketrek_errors';
let errLog = [];
let lastErr = '';
function logError(e, where) {
  const msg = (e && (e.message || e.reason || e)) + '';
  const key = where + msg;
  if (key === lastErr) return;  // el mismo error en cada cuadro: una sola vez
  lastErr = key;
  const trace = e && e.stack ? String(e.stack).split(/\n/).slice(0, 3).join(' | ') : '';
  errLog.push({ t: new Date().toISOString(), where, msg, trace, screens: screensPath() });
  if (errLog.length > 30) errLog = errLog.slice(-30);
  try { localStorage.setItem(ERRLOG_KEY, JSON.stringify(errLog)); } catch (x) { /* sin almacenamiento */ }
  try { console.error('[PokeTrek]', where, e); } catch (x) { /* nada */ }
  if (typeof Tele !== 'undefined') Tele.track('error', where, { msg: msg.slice(0, 200), screens: screensPath(), trace: trace.slice(0, 300) });
}
function screensPath() {
  try { return stack.map(s => s.constructor ? s.constructor.name : '?').join(' > '); } catch (x) { return ''; }
}
try { errLog = JSON.parse(localStorage.getItem(ERRLOG_KEY) || '[]'); } catch (e) { errLog = []; }
window.addEventListener('error', ev => logError(ev.error || ev.message, 'window'));
window.addEventListener('unhandledrejection', ev => logError(ev.reason, 'promise'));
