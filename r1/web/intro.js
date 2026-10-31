'use strict';
// Intro con el profesor, nombres del jugador y del rival, y los combates
// contra el rival (Blue por defecto) en los momentos clave del viaje.
// Los nombres viven en su propia clave de guardado: asi la partida del motor
// no cambia de formato y las partidas viejas siguen cargando.

const PROFILE_KEY = 'poketrek_profile';
const profile = { player: 'RED', rival: 'BLUE', rivalDone: 0 };
function saveProfile() { store.set(PROFILE_KEY, JSON.stringify(profile)); }
async function loadProfile() {
  try { Object.assign(profile, JSON.parse(await store.get(PROFILE_KEY) || '{}')); } catch (e) { /* por defecto */ }
}

// personaje de pixeles hecho con rectangulos (profesor o rival)
function drawPerson(x, y, kind, t) {
  const bob = Math.round(Math.sin(t / 500));
  const P = kind === 'prof'
    ? { hair: '#c8c8c8', top: '#f8f8f8', trim: '#c0c8d0', legs: '#806040', shoes: '#383028' }
    : { hair: '#a86838', top: '#7058a8', trim: '#504080', legs: '#383848', shoes: '#282830' };
  x = Math.round(x); y = Math.round(y) + bob;
  ctx.fillStyle = 'rgba(0,0,0,0.2)';
  ctx.beginPath(); ctx.ellipse(x, y + 1, 14, 3, 0, 0, Math.PI * 2); ctx.fill();
  rect(x - 7, y - 4, 6, 4, P.shoes); rect(x + 1, y - 4, 6, 4, P.shoes);
  rect(x - 6, y - 16, 5, 12, P.legs); rect(x + 1, y - 16, 5, 12, P.legs);
  rect(x - 9, y - 34, 18, 20, P.top);                       // cuerpo
  if (kind === 'prof') { rect(x - 9, y - 18, 18, 4, P.top); rect(x - 1, y - 34, 2, 18, P.trim); }  // bata
  rect(x - 12, y - 32, 4, 14, P.top); rect(x + 8, y - 32, 4, 14, P.top);  // brazos
  rect(x - 12, y - 19, 4, 3, '#f0c8a0'); rect(x + 8, y - 19, 4, 3, '#f0c8a0');
  rect(x - 7, y - 47, 14, 13, '#f0c8a0');                   // cara
  rect(x - 4, y - 42, 2, 2, '#282830'); rect(x + 2, y - 42, 2, 2, '#282830');
  rect(x - 8, y - 50, 16, 5, P.hair);                       // pelo
  if (kind === 'prof') { rect(x - 9, y - 47, 3, 6, P.hair); rect(x + 6, y - 47, 3, 6, P.hair); }
  else for (let i = 0; i < 4; i++) rect(x - 8 + i * 4, y - 53 + (i % 2) * 2, 3, 4, P.hair);  // pelo en punta
}

// ------------------------------------------------------------------ intro --

class IntroScreen {
  constructor(then) {
    this.then = then;
    this.step = 0;
    trainerImage('oak'); trainerImage('blue');
    this.script = [
      { who: 'prof', key: 'intro1' }, { who: 'prof', key: 'intro2' }, { who: 'prof', key: 'intro3' },
      { who: 'prof', key: 'intro4', name: 'player' },
      { who: 'rival', key: 'intro5', name: 'rival' },
      { who: 'rival', key: 'intro6' }, { who: 'prof', key: 'intro7' },
    ];
  }
  draw(t) {
    rect(0, 0, W, H, '#e8e0c8');
    for (let y = 150; y < H; y += 12) for (let x = 0; x < W; x += 16) rect(x, y, 16, 12, ((x / 16 + y / 12) % 2) ? '#d8c8a8' : '#c8b898');
    rect(0, 40, W, 6, '#a89878');
    for (let x = 12; x < W; x += 56) { rect(x, 52, 30, 40, '#7898b8'); rect(x + 2, 54, 26, 36, '#a8c8e8'); }  // ventanas del laboratorio
    const s = this.script[this.step];
    if (!drawTrainer(s.who === 'prof' ? 'oak' : 'blue', W / 2, 172)) drawPerson(W / 2, 168, s.who, t);
    const text = L(s.key).replace('{player}', profile.player).replace('{rival}', profile.rival);
    dialogBox(wrap(text, 26).slice(0, 5), t, true);
  }
  input(ev) {
    if (ev.k !== 'ok' && ev.k !== 'tap') return;
    const s = this.script[this.step];
    const advance = () => { if (++this.step >= this.script.length) { pop(); this.then(); } };
    if (s.name) pickName(s.name, advance);
    else advance();
  }
}

// nombre: unos predefinidos como en el juego, o escribirlo
function pickName(which, then) {
  const presets = which === 'player' ? ['RED', 'ASH', 'JACK'] : ['BLUE', 'GARY', 'JOHN'];
  const items = [{ label: L('newName'), action: () => { pop(); push(new KeyboardScreen(L(which === 'player' ? 'yourName' : 'rivalName'), '', n => { profile[which] = n; saveProfile(); then(); })); } }]
    .concat(presets.map(n => ({ label: n, action: () => { pop(); profile[which] = n; saveProfile(); then(); } })));
  push(new ListScreen(L(which === 'player' ? 'yourName' : 'rivalName'), items, { onBack: () => {} }));
}

// teclado con la rueda: mover entre letras, el boton escribe; o tocar las teclas
class KeyboardScreen {
  constructor(title, initial, onDone) {
    this.title = title;
    this.name = initial || '';
    this.onDone = onDone;
    this.keys = 'ABCDEFGHIJKLMNÑOPQRSTUVWXYZ'.split('').concat(['⌫', 'OK']);
    this.sel = 0;
  }
  keyRect(i) {
    if (i >= 27) return { x: i === 27 ? 20 : 130, y: 214, w: 90, h: 24 };
    return { x: 12 + (i % 9) * 24, y: 110 + Math.floor(i / 9) * 30, w: 22, h: 26 };
  }
  draw(t) {
    menuBg();
    header(this.title);
    roundBox(30, 40, W - 60, 46, BOX.light);
    for (let i = 0; i < 7; i++) {
      const ch = this.name[i] || '';
      textS(ch, 52 + i * 20, 52, INK, 'center');
      rect(46 + i * 20, 66, 12, 2, i === this.name.length && Math.floor(t / 400) % 2 ? '#c03028' : '#a0a8b0');
    }
    roundBox(6, 102, W - 12, 146, BOX.light);
    this.keys.forEach((k, i) => {
      const r = this.keyRect(i);
      if (i === this.sel) roundBox(r.x - 1, r.y - 1, r.w + 2, r.h + 2, BOX.sel);
      textS(k === '⌫' ? L('erase') : k, r.x + r.w / 2, r.y + (r.h - 8) / 2, INK, 'center');
    });
  }
  press(i) {
    const k = this.keys[i];
    if (k === '⌫') this.name = this.name.slice(0, -1);
    else if (k === 'OK') { if (this.name) { pop(); this.onDone(this.name); } }
    else if (this.name.length < 7) this.name += k;
    if (this.name.length >= 7 && k !== '⌫' && k !== 'OK') this.sel = 28;  // lleno: salta a OK
  }
  input(ev) {
    const n = this.keys.length;
    if (ev.k === 'up' || ev.k === 'left') this.sel = (this.sel + n - 1) % n;
    else if (ev.k === 'down' || ev.k === 'right') this.sel = (this.sel + 1) % n;
    else if (ev.k === 'ok') this.press(this.sel);
    else if (ev.k === 'back') { if (this.name) this.name = this.name.slice(0, -1); else pop(); }
    else if (ev.k === 'tap') {
      if (ev.y < 18) { pop(); return; }
      for (let i = 0; i < n; i++) {
        const r = this.keyRect(i);
        if (ev.x >= r.x && ev.x <= r.x + r.w && ev.y >= r.y && ev.y <= r.y + r.h) { this.sel = i; this.press(i); return; }
      }
    }
  }
}

// ------------------------------------------------------------------ rival --
// Etapas: 1a medalla, 20 km, 4 medallas, 6 medallas y antes de la Liga (8).

const RIVAL_STAGES = [
  s => popcount(s.badges) >= 1,
  s => s.meters >= 20000,
  s => popcount(s.badges) >= 4,
  s => popcount(s.badges) >= 6,
  s => popcount(s.badges) >= 8,
];
let rivalSnoozed = false;  // "despues": no vuelve a retar hasta abrir la app otra vez

function rivalStageDue() {
  for (let i = 0; i < RIVAL_STAGES.length; i++) {
    if (profile.rivalDone & (1 << i)) continue;
    return RIVAL_STAGES[i](S) ? i : -1;
  }
  return -1;
}

function checkRival() {
  if (rivalSnoozed || !S || !S.party.some(m => m.hp > 0)) return;
  const stage = rivalStageDue();
  if (stage < 0 || !(topScreen() instanceof HomeScreen)) return;
  push(new RivalScreen(stage));
}

class RivalScreen {
  constructor(stage) {
    this.stage = stage;
    this.t0 = performance.now();
    this.asked = false;
  }
  draw(t) {
    drawBattleBg('grass', t, S.night, 200);
    const slide = Math.max(0, 1 - (t - this.t0) / 500);
    if (!drawTrainer('blue', W / 2 + slide * 140, 156)) drawPerson(W / 2 + slide * 140, 150, 'rival', t);
    dialogBox(wrap(L('rivalChallenge', profile.rival), 26).slice(0, 3), t, !this.asked);
  }
  input(ev) {
    if (this.asked || (ev.k !== 'ok' && ev.k !== 'tap')) return;
    this.asked = true;
    ask(L('rivalChallenge', profile.rival), [L('battleIt'), L('later')], k => {
      if (k === 1) { rivalSnoozed = true; pop(); return; }
      pop();
      const stage = this.stage;
      startBattle(5, stage, r => {
        if (r && r.outcome === 1) {
          profile.rivalDone |= 1 << stage;
          saveProfile();
          message(L('rivalWin', profile.rival), () => topScreen().resume && topScreen().resume());
        } else {
          rivalSnoozed = true;
          message(L('rivalLose', profile.rival), () => topScreen().resume && topScreen().resume());
        }
      });
    });
  }
}
