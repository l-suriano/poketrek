'use strict';
// Pantallas fuera de combate. Todas leen el estado del motor (pt_state) y
// mandan las acciones con Engine.call; despues de cada cambio, saveSoon().

let S = null;  // ultimo pt_state
function refresh() { tick(); S = Engine.json('pt_state'); theme = S.night ? THEMES.night : THEMES.day; return S; }

const IT = { POKE_BALL: 0, GREAT_BALL: 1, ULTRA_BALL: 2, MASTER_BALL: 3, INCENSE: 29 };
const BALLS = [IT.POKE_BALL, IT.GREAT_BALL, IT.ULTRA_BALL, IT.MASTER_BALL];
const PK = { ITEMS: 0, BALLS: 1, MEDICINE: 2, KEY: 3 };

function fmtTime(sec) { sec = Math.max(0, sec); return `${Math.floor(sec / 60)}:${String(sec % 60).padStart(2, '0')}`; }
function money(n) { return '₽' + n.toLocaleString('en-US'); }
function partner() { return S.party.find(m => m.partner) || S.party[0]; }

// ------------------------------------------------------------- arranque ----

function langScreen(then) {
  return new ListScreen('LANGUAGE / IDIOMA', [
    { label: 'English', action: () => { setLang(LANG_EN); pop(); then(); } },
    { label: 'Español', action: () => { setLang(LANG_ES); pop(); then(); } },
  ], { onBack: () => {} });
}
function setLang(l) { lang = l; Engine.call('pt_set_lang', l); for (const k in typeNames) delete typeNames[k]; }

class StarterScreen {
  constructor() { this.sel = 1; this.mons = [1, 4, 7]; }
  draw(t) {
    menuBg();
    header(L('chooseStarter'));
    roundBox(3, 22, W - 6, 30, BOX.light);
    wrap(L('starterNote'), 27).slice(0, 2).forEach((l, i) => textS(l, W / 2, 28 + i * 11, INK, 'center'));
    this.mons.forEach((sp, i) => {
      const x = 40 + i * 80, y = 150;
      roundBox(x - 37, 60, 74, 128, i === this.sel ? BOX.sel : BOX.light);
      platform(x, y, 28, 6, 'grass', false);
      drawMon(sp, 'front', x, y, t, { scale: 1.6 });
      textS(spName(sp).slice(0, 8), x, y + 18, i === this.sel ? '#c03028' : INK, 'center');
    });
    dialogBox(wrap(L('chooseStarter'), 26).slice(0, 1).concat([spName(this.mons[this.sel]) + '?']), t, true);
  }
  input(ev) {
    if (ev.k === 'up' || ev.k === 'left') this.sel = (this.sel + 2) % 3;
    else if (ev.k === 'down' || ev.k === 'right') this.sel = (this.sel + 1) % 3;
    else if (ev.k === 'tap') { const i = Math.floor(ev.x / 80); if (i === this.sel) this.pick(); else this.sel = i; }
    else if (ev.k === 'ok') this.pick();
  }
  pick() {
    Engine.call('pt_new', this.mons[this.sel]);
    saveNow();
    replace(new HomeScreen());
  }
}

// ----------------------------------------------------------------- inicio --
// El companero en un mapa de pasto, con el menu "Start" de los juegos.

class HomeScreen {
  constructor() {
    this.menu = new ListScreen('', [], {});
    this.menu.draw = () => {};
  }
  enter() { this.build(); }
  resume() { this.build(); }
  build() {
    refresh();
    const sel = this.menu.sel;
    this.menu.items = [
      { label: L('explore'), right: S.events + S.drops ? '!' : '', action: () => push(exploreScreen()) },
      { label: L('pokemon'), action: () => push(partyScreen('menu')) },
      { label: L('bag'), action: () => push(bagScreen()) },
      { label: L('shop'), action: () => push(shopsScreen()) },
      { label: L('gyms'), action: () => push(gymsScreen()) },
      { label: L('tournaments'), right: S.dcPlayed ? '' : '!', action: () => push(tourScreen()) },
      { label: L('pokedex'), action: () => push(dexScreen()) },
      { label: L('settings'), action: () => push(settingsScreen()) },
    ];
    this.menu.sel = sel;
    checkPending(() => {});
  }
  draw(t) {
    menuBg();
    header('PokéTrek', money(S.money));
    drawOverworld(19, t, S.night);
    const p = partner();
    if (p) {
      ctx.fillStyle = 'rgba(0,0,0,0.25)';
      ctx.beginPath(); ctx.ellipse(70, 97, 12, 3, 0, 0, Math.PI * 2); ctx.fill();
      drawMon(p.sp, 'front', 70, 97, t, { scale: monScale(p.sp, p.hd, 'front', 34) });
    }
    // datos del entrenador
    roundBox(3, 134, 104, 145, BOX.light);
    if (p) { textS(p.name.slice(0, 10), 10, 142); textS('Lv' + p.lv, 10, 154, INK_DIM); }
    textS('EE', 10, 170);
    rect(30, 172, 70, 5, '#404848');
    rect(31, 173, Math.round(68 * S.ee / S.eeMax), 3, '#40a8f8');
    textS(String(S.ee), 100, 180, INK_DIM, 'right');
    textS((S.meters / 1000).toFixed(1) + 'km', 10, 194);
    textS('★' + popcount(S.badges), 10, 208, '#c09020');
    textS(S.caught + '/151', 100, 208, INK, 'right');
    textS(S.outside ? L('outside') : L('atHome'), 10, 224, S.outside ? '#c03028' : INK_DIM);
    textS(S.night ? L('night') : L('day'), 10, 238, INK_DIM);
    if (S.events + S.drops) textS('!' + (S.events + S.drops), 100, 224, '#c03028', 'right');
    // menu Start
    const m = this.menu;
    roundBox(109, 134, 128, 145, BOX.light);
    m.items.forEach((it, i) => {
      const y = 142 + i * 17;
      if (i === m.sel) cursor(116, y, INK);
      textS(it.label.slice(0, 12), 126, y);
      if (it.right) textS(it.right, 230, y, '#c03028', 'right');
    });
  }
  input(ev) {
    if (ev.k === 'back') return;
    if (ev.k === 'tap') {
      const i = Math.floor((ev.y - 140) / 17);
      if (ev.x > 109 && i >= 0 && i < this.menu.items.length) { if (i === this.menu.sel) this.menu.choose(i); else this.menu.sel = i; }
      return;
    }
    this.menu.input(ev);
  }
}

function popcount(n) { let c = 0; while (n) { c += n & 1; n >>= 1; } return c; }

// ---------------------------------------------------------------- explorar -

function exploreScreen() {
  const build = () => {
    refresh();
    const inc = S.incense ? fmtTime(S.incenseUntil - nowSec()) : (S.bag.find(b => b.id === IT.INCENSE) || { n: 0 }).n;
    const healing = S.healAt ? fmtTime(S.healAt - nowSec()) : '';
    return [
      { label: L('walking'), right: S.outside ? L('outside') : L('atHome'), rightColor: S.outside ? theme.sel : theme.dim,
        action: () => { Engine.call('pt_set_outside', S.outside ? 0 : 1); saveSoon(); s.resume(); } },
      { label: L('outsideEvents'), right: S.events + S.drops, rightColor: theme.sel,
        action: () => { if (!S.events && !S.drops) message(L('noEvents')); else nextOutsideEvent(); } },
      { label: L('searchRoute'), right: '300 EE', action: () => push(routesScreen()) },
      { label: L('incense'), right: inc, action: () => S.incense ? message(L('incenseOn')) : push(routesScreen(true)) },
      { label: L('eggs'), right: S.eggs.filter(e => e.tier).length + '/3', action: () => push(eggsScreen()) },
      { label: L('center'), right: healing ? L('healing') + ' ' + healing : '', action: () => {
        const r = Engine.call('pt_heal', S.outside ? 0 : 1);
        saveSoon();
        message(r === 1 ? L('healHome') : L('healAway'), () => s.resume());
      } },
      { label: L('testSteps'), action: () => { walk(500); s.resume(); } },
    ];
  };
  const s = new ListScreen(L('explore'), build(), {
    y0: 50, refresh: build,
    drawTop: () => {
      text('EE ' + S.ee + '/' + S.eeMax, 6, 23);
      bar(6, 33, W - 12, 4, S.ee / S.eeMax, theme.accent);
      text(S.outside ? (lang === LANG_ES ? `Evento en ${S.toEvent} pasos` : `Event in ${S.toEvent} steps`) : (S.steps + ' ' + (lang === LANG_ES ? 'pasos' : 'steps')), 6, 40, theme.dim);
    },
  });
  return s;
}

function walk(n) {
  const r = Engine.json('pt_walk', n, S.outside ? 1 : 0);
  saveSoon();
  return r;
}

function nextOutsideEvent() {
  const e = Engine.json('pt_next_event');
  encounterBg = 'grass';
  saveSoon();
  handleEvent(e);
}

function handleEvent(e) {
  if (e.kind === 'wild' || e.kind === 'legend' || e.kind === 'trainer') push(new EncounterScreen(e));
  else if (e.kind === 'item') message(L('foundItem', e.itemName, e.n));
  else if (e.kind === 'drop') showLoot(e.loot, L('supplyDrop'));
  else if (e.kind === 'nothing') message(L('foundNothing'));
  else if (e.kind === 'error') message(L('needEE'));
}

function showLoot(loot, title) {
  const lines = [title, L('gotItems')];
  for (const it of loot.items) lines.push(`${it.n}× ${it.name}`);
  if (loot.berries) lines.push(`${loot.berries}× ${L('berries')}`);
  if (loot.egg) lines.push(L('gotEgg', L('eggTier')[loot.egg]));
  message(lines, () => { if (loot.merchant) push(merchantScreen(loot)); });
}

function merchantScreen(loot) {
  const items = loot.offer.map((o, i) => ({ label: o.name, right: money(o.price), action: () => {
    const ok = Engine.call('pt_merchant_buy', i);
    saveSoon();
    message(ok ? L('bought') : L('shopErr')[2]);
  } }));
  items.push({ label: L('leave'), action: () => pop() });
  return new ListScreen(L('merchant'), items, { right: () => money(S.money) });
}

function routesScreen(forIncense) {
  const routes = Engine.json('pt_routes');
  const items = routes.map((r, i) => ({
    label: r.name,
    right: r.open ? (forIncense ? '' : '') : (r.badges === 9 ? L('afterChampion') : r.badges + ' ★'),
    disabled: !r.open,
    why: L('routeLocked'),
    action: () => {
      if (forIncense) {
        const ok = Engine.call('pt_incense_start', i);
        saveSoon();
        message(ok ? L('incenseStarted') : L('incenseNeed'), () => pop());
        return;
      }
      const e = Engine.json('pt_search', i);
      encounterBg = ROUTE_BG[i] || 'grass';
      saveSoon();
      handleEvent(e);
    },
  }));
  return new ListScreen(forIncense ? L('incense') : L('searchRoute'), items, { right: () => 'EE ' + S.ee });
}

function eggsScreen() {
  const build = () => {
    refresh();
    const items = [];
    S.eggs.forEach((e, i) => {
      if (!e.tier) return;
      const ready = e.walked >= e.goal;
      items.push({ label: L('eggs').slice(0, -1) + ' ' + L('eggTier')[e.tier], right: ready ? L('eggReady') : (e.walked / 1000).toFixed(1) + '/' + (e.goal / 1000) + ' km',
        rightColor: ready ? theme.sel : theme.dim, hp: e.walked / e.goal,
        action: () => {
          if (!ready) return;
          const sp = Engine.call('pt_hatch', i);
          saveSoon();
          if (sp) message(L('hatched', JSON.parse(Engine.call('pt_species_name', sp))), () => s.resume());
          else message(L('boxFull'));
        } });
    });
    return items;
  };
  const s = new ListScreen(L('eggs'), build(), { refresh: build, footer: () => S.eggs.some(e => e.tier) ? '' : L('noEggs') });
  return s;
}

// ------------------------------------------------------------- encuentro ---

class EncounterScreen {
  constructor(e) {
    this.e = e;
    this.t0 = performance.now();
    if (e.wild) spriteImage(e.wild.sp);  // que el sprite ya este cargado al aparecer
    (e.party || []).forEach(m => spriteImage(m.sp));
    const trainer = e.kind === 'trainer';
    const opts = trainer ? [L('battleIt'), L('decline')] : [L('throwBall'), L('battleIt'), L('run')];
    this.menu = opts;
    this.sel = 0;
  }
  draw(t) {
    const e = this.e;
    drawBattleBg(encounterBg, t, S.night, 196);
    platform(120, 146, 64, 12, encounterBg, S.night);
    if (e.kind === 'trainer') {
      e.party.forEach((m, i) => drawMon(m.sp, 'front', 60 + i * 60, 146, t, { scale: monScale(m.sp, m.hd || 10, 'front', 34) }));
      roundBox(30, 8, W - 60, 22, BOX.info);
      textS(e.cls, W / 2, 15, INK, 'center');
    } else {
      const w = e.wild;
      const intro = Math.min(1, (t - this.t0) / 500);
      drawMon(w.sp, 'front', 120, 146, t, { scale: monScale(w.sp, w.hd, 'front', 44), alpha: intro });
      if (encounterBg === 'grass') tallGrass(140, t, S.night);
      if (e.shiny && Math.floor(t / 300) % 2) textS('✦', 150, 70, '#f8d030', 'center', '#806010');
      roundBox(4, 8, 132, 22, BOX.info);
      textS(w.name, 11, 15);
      textS('Lv' + w.lv, 129, 15, INK, 'right');
      if (e.expires) { roundBox(W - 52, 8, 48, 22, BOX.info); textS(fmtTime(e.expires - nowSec()), W - 28, 15, INK, 'center'); }
    }
    const msg = e.kind === 'trainer' ? L('trainerWants', e.cls) : e.kind === 'legend' ? L('legendAppeared', e.wild.name) : L('wildAppeared', e.wild.name);
    roundBox(0, 196, W, 86, BOX.dark);
    wrap(msg, 26).slice(0, 2).forEach((l, i) => textS(l, 10, 204 + i * 12, '#f8f8f8', 'left', '#182030'));
    roundBox(4, 230, W - 8, 48, BOX.light);
    this.menu.forEach((o, i) => {
      const x = 20 + (i % 2) * 112, y = 238 + Math.floor(i / 2) * 20;
      if (i === this.sel) cursor(x - 10, y, INK);
      textS(o, x, y);
    });
  }
  input(ev) {
    const n = this.menu.length;
    if (ev.k === 'up') this.sel = (this.sel + n - 1) % n;
    else if (ev.k === 'down') this.sel = (this.sel + 1) % n;
    else if (ev.k === 'tap' && ev.y > 230) { const i = (ev.x > W / 2 ? 1 : 0) + (ev.y > 256 ? 2 : 0); if (i < n) { this.sel = i; this.pick(); } }
    else if (ev.k === 'ok') this.pick();
  }
  leave() { /* nada */ }
  pick() {
    const e = this.e;
    if (e.kind === 'trainer') {
      if (this.sel === 0) startBattle(1, 0, () => pop());
      else { Engine.call('pt_leave_encounter'); pop(); }
      return;
    }
    if (this.sel === 0) {
      chooseBall(ball => push(new ThrowScreen({
        sp: e.wild.sp, hd: e.wild.hd, ball,
        resolve: (hit, x10) => Engine.json('pt_throw', ball, x10, hit ? 1 : 0),
        done: r => {
          saveSoon();
          if (r.res === 'caught' || r.res === 'full' || r.res === 'fled') {
            pop();  // la pantalla del lanzamiento
            message(r.res === 'caught' ? L('caught', e.wild.name) : r.res === 'full' ? L('boxFull') : L('fled', e.wild.name), () => pop());
          } else {
            pop();
            if (r.res === 'noball') message(L('noBalls'));
          }
        },
      })));
    } else if (this.sel === 1) {
      startBattle(1, 0, () => pop());
    } else {
      Engine.call('pt_leave_encounter');
      saveSoon();
      pop();
    }
  }
}

function chooseBall(then) {
  refresh();
  const balls = BALLS.map(id => S.bag.find(b => b.id === id)).filter(Boolean);
  if (!balls.length) { message(L('noBalls')); return; }
  if (balls.length === 1) { then(balls[0].id); return; }
  push(new ListScreen(L('chooseBall'), balls.map(b => ({ label: b.name, right: '×' + b.n, action: () => { pop(); then(b.id); } }))));
}

// ------------------------------------------------------------ lanzamiento --
// Inclinar el R1 mueve la mira; girar la rueda hacia arriba rapido lanza.
// El aro que se encoge da el bono: Nice x1,3, Great x1,6, Excellent x2.

class ThrowScreen {
  constructor(o) {
    this.o = o;
    this.phase = 'aim';
    this.t0 = performance.now();
    this.aim = { x: W / 2, y: 120 };
    input.wheelUps = [];
    this.label = '';
  }
  ring(t) {
    const p = ((t - this.t0) % 1500) / 1500;
    return 36 - 28 * p;  // de 36 a 8 px
  }
  target(t) { return { x: W / 2 + Math.sin(t / 700) * 34, y: 118 + Math.sin(t / 430) * 6 }; }
  draw(t) {
    drawBattleBg(encounterBg, t, S.night, 200);
    platform(W / 2, 140, 64, 12, encounterBg, S.night);
    const tg = this.phase === 'aim' ? this.target(t) : this.frozen;
    const visible = this.phase === 'aim' || this.phase === 'fly' || (this.phase === 'result' && this.result && this.result.res !== 'caught');
    if (visible) drawMon(this.o.sp, 'front', tg.x, tg.y + 22, t, { scale: monScale(this.o.sp, this.o.hd, 'front', 44) });
    if (encounterBg === 'grass') tallGrass(134, t, S.night);
    if (this.phase === 'aim') {
      // aro de bono
      const r = this.ring(t);
      const f = (r - 8) / 28;
      ctx.strokeStyle = f < 0.2 ? '#e8503a' : f < 0.45 ? '#f0a040' : f < 0.7 ? '#f8d030' : '#58b868';
      ctx.lineWidth = 2;
      ctx.beginPath(); ctx.arc(tg.x, tg.y, r, 0, Math.PI * 2); ctx.stroke();
      // mira
      this.updateAim();
      ctx.strokeStyle = theme.ink;
      ctx.lineWidth = 1;
      ctx.beginPath(); ctx.arc(this.aim.x, this.aim.y, 6, 0, Math.PI * 2); ctx.stroke();
      rect(this.aim.x - 9, this.aim.y, 18, 1, theme.ink);
      rect(this.aim.x, this.aim.y - 9, 1, 18, theme.ink);
      this.checkFlick(t, tg, r);
    }
    if (this.phase === 'fly') {
      const p = Math.min(1, (t - this.flyT) / 450);
      const bx = W / 2 + (this.aim.x - W / 2) * p, by = 260 + (this.aim.y - 260) * p - Math.sin(p * Math.PI) * 60;
      drawBall(bx, by, this.o.ball, p * 10);
      if (p >= 1) this.land(t);
    }
    if (this.phase === 'shake') {
      const k = Math.floor((t - this.shakeT) / 600);
      const wob = k < this.result.shakes ? Math.sin((t - this.shakeT) / 60) * 4 : 0;
      drawBall(this.frozen.x + wob, this.frozen.y + 18, this.o.ball, 0);
      if (k > this.result.shakes) { this.phase = 'result'; this.resT = t; }
    }
    if (this.phase === 'result') {
      if (this.result.res === 'caught') { drawBall(this.frozen.x, this.frozen.y + 18, this.o.ball, 0); text('★', this.frozen.x + 14, this.frozen.y, '#f0c020'); }
      if (t - this.resT > 900) { this.phase = 'done'; this.o.done(this.result); }
    }
    // panel
    roundBox(0, 200, W, 82, BOX.dark);
    roundBox(4, 206, W - 8, 72, BOX.light);
    const msg = this.phase === 'aim' ? (input.tilt.ok ? L('aimHint') : L('aimHintDesk'))
              : this.phase === 'result' || this.phase === 'done' ? this.resultText() : this.label;
    wrap(msg, 26).slice(0, 4).forEach((l, i) => textS(l, 12, 214 + i * 12));
    const b = S.bag.find(x => x.id === this.o.ball);
    textS((b ? b.name : '') + ' ×' + (b ? b.n : 0), 12, 264, INK_DIM);
  }
  resultText() {
    const r = this.result || {};
    const name = spName(this.o.sp);
    if (r.res === 'short') return L('short');
    if (r.res === 'miss') return L('missed');
    if (r.res === 'caught') return (this.label ? this.label + '! ' : '') + L('caught', name);
    if (r.res === 'fled') return L('fled', name);
    if (r.res === 'broke') return (this.label ? this.label + '! ' : '') + L('brokeFree');
    return '';
  }
  updateAim() {
    if (input.tilt.ok) {
      const sx = prefs.invertX ? -1 : 1, sy = prefs.invertY ? -1 : 1;
      const tx = W / 2 + input.tilt.x * 120 * sx, ty = 118 - input.tilt.y * 100 * sy;
      this.aim.x += (tx - this.aim.x) * 0.25;
      this.aim.y += (ty - this.aim.y) * 0.25;
    } else if (input.mouse) {
      this.aim.x = input.mouse.x;
      this.aim.y = Math.min(190, input.mouse.y);
    }
    this.aim.x = Math.max(6, Math.min(W - 6, this.aim.x));
    this.aim.y = Math.max(24, Math.min(194, this.aim.y));
  }
  // rafaga de la rueda hacia arriba: 3+ pasos en 350 ms lanza; 2 se queda corta
  checkFlick(t, tg, r) {
    const now = performance.now();
    const ups = input.wheelUps.filter(x => now - x < 350);
    if (!ups.length) { input.wheelUps = []; return; }
    if (now - ups[ups.length - 1] < 160) return;  // la rafaga sigue
    input.wheelUps = [];
    if (ups.length >= 3) this.throwBall(t, tg, r, false);
    else if (ups.length === 2) this.throwBall(t, tg, r, true);
  }
  throwBall(t, tg, r, short) {
    this.frozen = { ...tg };
    const f = (r - 8) / 28;
    this.x10 = f < 0.2 ? 20 : f < 0.45 ? 16 : f < 0.7 ? 13 : 10;
    this.label = this.x10 === 20 ? L('throwNames')[3] : this.x10 === 16 ? L('throwNames')[2] : this.x10 === 13 ? L('throwNames')[1] : '';
    this.hit = !short && Math.hypot(this.aim.x - tg.x, this.aim.y - tg.y) <= 22;
    if (short) { this.aim = { x: this.aim.x, y: 175 }; this.hit = false; }
    this.short = short;
    this.phase = 'fly';
    this.flyT = t;
  }
  land(t) {
    this.result = this.o.resolve(this.hit, this.x10);
    refresh();  // bolas que quedan
    if (this.short && this.result.res === 'miss') this.result.res = 'short';
    if (this.result.res === 'noball' || this.result.res === 'none') { this.phase = 'done'; this.o.done(this.result); return; }
    if (this.result.res === 'miss' || this.result.res === 'short') { this.phase = 'result'; this.resT = t; return; }
    this.phase = 'shake';
    this.shakeT = t;
  }
  input(ev) {
    if (this.phase !== 'aim') return;
    if (ev.k === 'back') { pop(); return; }
    if (ev.k === 'tap' && !input.tilt.ok) {
      // en el PC o sin acelerometro: tocar lanza hacia donde se toca
      this.aim = { x: ev.x, y: Math.min(194, ev.y) };
      const t = performance.now();
      this.throwBall(t, this.target(t), this.ring(t), false);
    }
    if (ev.k === 'ok' && !input.tilt.ok && ev.key === ' ') {
      const t = performance.now();
      this.throwBall(t, this.target(t), this.ring(t), false);
    }
  }
}

function drawBall(x, y, ball, spin) {
  const colors = ['#e8503a', '#3a78d8', '#2a2a36', '#a040c0'];
  ctx.save();
  ctx.translate(Math.round(x), Math.round(y));
  ctx.rotate(spin);
  ctx.fillStyle = colors[ball] || '#e8503a';
  ctx.beginPath(); ctx.arc(0, 0, 6, Math.PI, 0); ctx.fill();
  ctx.fillStyle = '#fff';
  ctx.beginPath(); ctx.arc(0, 0, 6, 0, Math.PI); ctx.fill();
  ctx.strokeStyle = '#2a2a36'; ctx.lineWidth = 1;
  ctx.beginPath(); ctx.arc(0, 0, 6, 0, Math.PI * 2); ctx.stroke();
  rect(-6, 0, 12, 1, '#2a2a36');
  rect(-1, -1, 3, 3, '#fff');
  ctx.restore();
}

// ----------------------------------------------------------------- equipo --

// mode: menu | switch | target (objeto) | teach (MT)
function partyScreen(mode, opts = {}) {
  const build = () => {
    refresh();
    return S.party.map((m, i) => ({
      label: m.name + (m.shiny ? ' ✦' : ''), right: 'Lv' + m.lv, icon: m.sp,
      sub: `${m.hp}/${m.max}` + (m.st ? ' ' + STATUS_TXT[lang === LANG_ES ? 0 : 1][m.st] : ''),
      hp: m.hp / m.max,
      disabled: mode === 'teach' && !Engine.call('pt_can_learn_tm', opts.tm, i),
      why: L('cantLearn'),
      action: () => opts.pick ? opts.pick(i, m) : monMenu(i, m),
    }));
  };
  return new ListScreen(opts.title || L('pokemon'), build(), { rowH: 26, refresh: build });
}

function monMenu(i, m) {
  const items = [
    { label: L('summary'), action: () => { pop(); push(new SummaryScreen(0, i)); } },
    { label: L('swap'), action: () => { pop(); push(partyScreen('swap', { title: L('swapWith'), pick: j => { Engine.call('pt_party_swap', i, j); saveSoon(); pop(); } })); } },
    { label: L('useItem'), action: () => { pop(); push(bagScreen({ target: i })); } },
    { label: L('toPC'), action: () => {
      pop();
      if (m.partner) { message(L('partnerStays')); return; }
      if (S.party.length <= 1) { message(L('lastOne')); return; }
      Engine.call('pt_deposit', i);
      saveSoon();
      topScreen().resume();
    } },
    { label: L('pc') + ' (' + S.boxCount + ')', action: () => { pop(); push(boxScreen()); } },
  ];
  push(new ListScreen(m.name, items));
}

function boxScreen() {
  const build = () => {
    refresh();
    return Engine.json('pt_box').map((m, i) => ({
      label: m.name + (m.shiny ? ' ✦' : ''), right: 'Lv' + m.lv, icon: m.sp, sub: `${m.hp}/${m.max}`,
      action: () => {
        ask(m.name, [L('withdraw'), L('summary'), L('back')], k => {
          if (k === 0) {
            if (S.party.length >= 6) { message(L('partyFull')); return; }
            Engine.call('pt_withdraw', i);
            saveSoon();
            topScreen().resume();
          } else if (k === 1) push(new SummaryScreen(1, i));
        });
      },
    }));
  };
  return new ListScreen(L('pc'), build(), { rowH: 26, refresh: build, right: () => S.boxCount + '/60' });
}

class SummaryScreen {
  constructor(inBox, idx) { this.m = Engine.json('pt_mon', inBox, idx); }
  draw(t) {
    const m = this.m;
    menuBg();
    header(m.name + (m.shiny ? ' ✦' : ''), 'Lv' + m.lv);
    // retrato sobre su fondo, como la ficha de los juegos
    roundBox(3, 22, 94, 84, BOX.light);
    ctx.save(); ctx.beginPath(); ctx.rect(6, 25, 88, 78); ctx.clip();
    ctx.translate(6, 25); ctx.scale(88 / W, 78 / 176); drawBattleBg('grass', t, S.night, 176); ctx.restore();
    platform(50, 96, 30, 6, 'grass', S.night);
    drawMon(m.sp, 'front', 50, 96, t, { scale: monScale(m.sp, m.hd, 'front', 36) });
    roundBox(99, 22, 138, 84, BOX.light);
    let x = typeChip(m.t1, 106, 28);
    if (m.t2 !== 255) typeChip(m.t2, 110 + x, 28);
    textS(`HP ${m.hp}/${m.max}`, 106, 44);
    bar(106, 54, 124, 4, m.hp / m.max);
    statusChip(m.st, 204, 42);
    textS(L('exp'), 106, 63, INK_DIM);
    bar(136, 65, 94, 3, (m.exp - m.expLv) / Math.max(1, m.expNext - m.expLv), '#40a8f8');
    textS(L('trust') + ' ' + m.trust, 106, 76);
    bar(106, 88, 124, 3, m.trust / 100, '#c070d0');
    roundBox(3, 108, W - 6, 32, BOX.light);
    textS(L('stats'), 12, 114, INK_DIM);
    textS([m.atk, m.def, m.spe, m.spc].map(v => String(v).padStart(3)).join(' '), 12, 126);
    m.moves.forEach((mv, i) => {
      const y = 142 + i * 34;
      roundBox(3, y, W - 6, 32, BOX.light);
      textS(mv.name, 11, y + 6);
      typeChip(mv.type, 11, y + 17);
      textS(`PP ${mv.pp}/${mv.max}`, W - 11, y + 6, INK, 'right');
      if (mv.pow) textS(String(mv.pow), W - 11, y + 19, INK_DIM, 'right');
    });
  }
  input(ev) { if (ev.k === 'back' || ev.k === 'ok' || ev.k === 'tap') pop(); }
}

// ---------------------------------------------------------------- mochila --

// opts.target: usar directamente en ese Pokemon; opts.inBattle: callback(item) para el combate
function bagScreen(opts = {}) {
  const names = L('pockets');
  const pockets = [PK.ITEMS, PK.BALLS, PK.MEDICINE, PK.KEY, 'tm'];
  const items = pockets.map((p, k) => ({
    label: names[k],
    right: () => '',
    action: () => push(pocketScreen(p, names[k], opts)),
  }));
  const s = new ListScreen(L('bag'), items, { right: () => money(S.money) });
  s.items.forEach((it, k) => {
    const p = pockets[k];
    Object.defineProperty(it, 'right', { get: () => p === 'tm' ? S.tms.length : S.bag.filter(b => b.pocket === p).length });
  });
  return s;
}

function pocketScreen(pocket, title, opts) {
  const build = () => {
    refresh();
    if (pocket === 'tm') {
      return S.tms.map(tm => ({ label: (tm.num > 50 ? 'HM' + String(tm.num - 50).padStart(2, '0') : 'TM' + String(tm.num).padStart(2, '0')) + ' ' + tm.name,
        right: tm.num > 50 ? '' : '×' + tm.n, action: () => teachFlow(tm) }));
    }
    return S.bag.filter(b => b.pocket === pocket).map(b => ({ label: b.name, right: '×' + b.n, action: () => useFlow(b, opts) }));
  };
  return new ListScreen(title, build(), { refresh: build });
}

function useFlow(item, opts) {
  if (opts.inBattle) { opts.inBattle(item); return; }
  if (item.pocket === PK.BALLS) { message(L('noEffect')); return; }
  const apply = i => {
    const r = Engine.json('pt_use_item', item.id, i);
    saveSoon();
    const name = S.party[i].name;
    const msg = r.res === 'ok' ? L('used', item.name) : r.res === 'evolved' ? L('evolved', name, r.to)
              : r.res === 'levelup' ? L('grewTo', name, r.lv) : L('noEffect');
    message(msg, () => checkPending(() => topScreen().resume && topScreen().resume()));
  };
  if (opts.target !== undefined) apply(opts.target);
  else push(partyScreen('target', { title: L('onWhich'), pick: i => { pop(); apply(i); } }));
}

function teachFlow(tm) {
  push(partyScreen('teach', { tm: tm.num, title: L('onWhich'), pick: (i, m) => {
    const mon = Engine.json('pt_mon', 0, i);
    const done = slot => {
      const ok = Engine.call('pt_teach', tm.num, i, slot);
      saveSoon();
      pop();
      message(ok ? L('learned', m.name, tm.name) : L('cantLearn'));
    };
    if (mon.moves.length < 4) done(-1);
    else ask(L('forgetWhich'), mon.moves.map(x => x.name).concat([L('back')]), k => { if (k < 4) done(k); });
  } }));
}

// ----------------------------------------------------------------- tiendas -

function shopsScreen() {
  return new ListScreen(L('shop'), [
    { label: L('mart'), action: () => push(shopScreen(0, L('mart'))) },
    { label: L('celadon'), action: () => push(shopScreen(1, L('celadon'))) },
    { label: L('battleShop'), action: () => push(shopScreen(3, L('battleShop'))) },
    { label: L('sell'), action: () => push(shopScreen(2, L('sell'))) },
  ], { right: () => money(S.money) });
}

function shopScreen(which, title) {
  const build = () => {
    refresh();
    const d = Engine.json('pt_shop', which);
    if (which === 1 && !d.open) return [{ label: L('celadonClosed'), disabled: true }];
    const items = d.items.map(it => {
      if (which === 3) {
        return { label: (it.tm ? it.label + ' ' : '') + it.name, right: it.bp + ' BP', action: () => {
          const r = Engine.call('pt_buy', 3, it.idx, 1);
          saveSoon();
          message(r === 0 ? L('bought') : L('shopErr')[r]);
        } };
      }
      return { label: it.name, right: money(it.price), sub: which === 2 ? '' : '', action: () => {
        const max = which === 2 ? it.have : Math.max(1, Math.min(99 - it.have, Math.floor(d.money / Math.max(1, it.price))));
        if (which !== 2 && d.money < it.price) { message(L('shopErr')[2]); return; }
        askNumber(L('howMany'), Math.max(1, max), n => money(n * it.price), n => {
          const r = Engine.call('pt_buy', which, it.id, n);
          saveSoon();
          message(r === 0 ? (which === 2 ? L('sold') : L('bought')) : L('shopErr')[r], () => topScreen().resume && topScreen().resume());
        });
      } };
    });
    if (which === 1) {
      for (const tm of d.tms) items.push({ label: 'TM' + String(tm.num).padStart(2, '0') + ' ' + tm.name, right: money(tm.price), action: () => {
        const r = Engine.call('pt_buy', 4, tm.slot, 1);
        saveSoon();
        message(r === 0 ? L('bought') : L('shopErr')[r]);
      } });
    }
    return items;
  };
  return new ListScreen(title, build(), { refresh: build, right: () => which === 3 ? S.bp + ' BP' : money(S.money) });
}

// -------------------------------------------------------------- gimnasios --

function gymsScreen() {
  let km = 0;
  const build = () => {
    const g = Engine.json('pt_gyms');
    km = g.km;
    const items = g.gyms.map((gym, i) => ({
      label: gym.leader, right: gym.badge ? '★' : gym.open ? typeName(gym.type) : L('locked'),
      rightColor: gym.badge ? '#f0c020' : theme.dim,
      sub: gym.open ? '' : (i > 0 && !g.gyms[i - 1].badge ? L('needBadge') : L('kmToOpen', gym.km - g.km)),
      disabled: !gym.open,
      action: () => ask(gym.leader + ' · ' + typeName(gym.type), [L('challenge'), L('back')], k => {
        if (k === 0) startBattle(2, i, () => topScreen().resume());
      }),
    }));
    items.push({
      label: L('league'), right: g.leagueActive ? (g.leagueIdx + 1) + '/5' : '', sub: g.leagueOpen ? (g.leagueActive ? L('leagueNext', g.elite[g.leagueIdx].name) : '') : L('leagueNeed'),
      disabled: !g.leagueOpen,
      action: () => {
        const opts = g.leagueActive ? [L('leagueNext', g.elite[g.leagueIdx].name), L('pokemon'), L('bag'), L('leagueQuit')] : [L('challenge'), L('back')];
        ask(L('league'), opts, k => {
          if (k === 0) startBattle(3, 0, () => topScreen().resume());
          else if (g.leagueActive && k === 1) push(partyScreen('menu'));
          else if (g.leagueActive && k === 2) push(bagScreen());
          else if (g.leagueActive && k === 3) { Engine.call('pt_league_quit'); saveSoon(); topScreen().resume(); }
        });
      },
    });
    return items;
  };
  return new ListScreen(L('gyms'), build(), { rowH: 26, refresh: build, right: () => km + ' km' });
}

// ----------------------------------------------------------------- torneos -

function tourScreen() {
  const build = () => {
    refresh();
    const d = Engine.json('pt_tour_info');
    const items = [];
    if (d.active && !d.over) items.push({ label: L('continueTour'), right: d.wins + '/3', rightColor: theme.sel, action: () => push(new TourRunScreen()) });
    if (d.active && d.over) items.push({ label: L('continueTour'), action: () => finishTour() });
    const rule = L('dailyRule')[d.daily.rule].replace('{0}', typeName(d.daily.type));
    items.push({ label: L('daily'), right: d.daily.played ? '✓' : '!', rightColor: d.daily.played ? theme.dim : theme.sel,
      sub: rule + (d.daily.prize ? ' · ' + d.daily.prize : ''),
      disabled: d.daily.played || (d.active && !d.over), why: L('dailyDone'),
      action: () => pickTeam(rule, picks => {
        const r = Engine.call('pt_daily_enter', ...picks);
        saveSoon();
        if (r) message(L('tourErr')[r]); else push(new TourRunScreen());
      }) });
    d.cups.forEach((c, i) => {
      if (i === 7 && !S.champion) return;
      const feat = i === d.featured;
      items.push({ label: c.name, sub: c.city + (feat ? ' · ' + L('featured') + (d.featuredType !== 255 ? ' ' + typeName(d.featuredType) : '') : ''),
        right: feat ? '★' : '', rightColor: '#f0c020', disabled: d.active && !d.over,
        action: () => push(tierScreen(i, c.name, d, feat)) });
    });
    return items;
  };
  return new ListScreen(L('tournaments'), build(), { rowH: 26, refresh: build, right: () => S.bp + ' BP' });
}

function tierScreen(cup, name, d, feat) {
  const items = d.tiers.map((t, k) => {
    const fee = feat ? t.fee * 3 / 4 : t.fee;
    return { label: t.name, right: money(fee), sub: `${t.champBp * (feat ? 2 : 1)} BP`, disabled: cup === 7 && k !== 3,
      action: () => pickTeam(name, picks => {
        const r = Engine.call('pt_tour_enter', cup, k, ...picks);
        saveSoon();
        if (r) message(L('tourErr')[r]); else { pop(); push(new TourRunScreen()); }
      }, cup === 6) };
  });
  return new ListScreen(name, items, { rowH: 26 });
}

// elige 3 del equipo y del PC; picks = (enPC << 7) | indice
function pickTeam(title, then, randomCup) {
  if (randomCup) { then([0, 0, 0]); return; }
  refresh();
  const box = Engine.json('pt_box');
  const all = S.party.map((m, i) => ({ m, code: i })).concat(box.map((m, i) => ({ m, code: 128 | i })));
  const chosen = [];
  const s = new ListScreen('', [], { rowH: 24 });
  const build = () => all.map(({ m, code }) => ({
    label: (chosen.includes(code) ? '● ' : '○ ') + m.name, right: 'Lv' + m.lv, icon: m.sp,
    action: () => {
      const k = chosen.indexOf(code);
      if (k >= 0) chosen.splice(k, 1); else if (chosen.length < 3) chosen.push(code);
      s.items = build();
      s.title = L('pickTeam', chosen.length);
      if (chosen.length === 3) ask(title, [L('yes'), L('no')], a => { if (a === 0) { pop(); then(chosen.slice()); } });
    },
  }));
  s.items = build();
  s.title = L('pickTeam', 0);
  push(s);
}

class TourRunScreen {
  enter() { this.d = Engine.json('pt_tour_info'); }
  resume() { this.d = Engine.json('pt_tour_info'); if (!this.d.active) pop(); }
  draw(t) {
    const d = this.d;
    drawBattleBg('stadium', t, false, H);
    header(d.isDaily ? L('daily') : (d.cups[d.cup] || {}).name || '', (d.tiers[d.tier] || {}).name || '');
    for (let r = 0; r < 3; r++) {
      const y = 60 + r * 36;
      const done = r < d.wins, cur = r === d.wins && !d.over;
      roundBox(20, y, W - 40, 30, cur ? BOX.sel : BOX.light);
      if (cur) cursor(28, y + 11, INK);
      textS(L('round')[r], 40, y + 11, done ? INK_DIM : INK);
      if (done) textS('✓', W - 32, y + 11, '#40a050', 'right');
    }
    roundBox(70, 172, 100, 22, BOX.info);
    textS('Lv ' + d.oppLevel, W / 2, 179, INK, 'center');
    roundBox(20, 228, W - 40, 32, BOX.light);
    cursor(32, 240, INK);
    textS(d.over ? L('continueTour') : L('battleIt'), W / 2 + 6, 240, INK, 'center');
  }
  input(ev) {
    if (ev.k === 'back') { pop(); return; }
    if (ev.k === 'ok' || (ev.k === 'tap' && ev.y > 220)) {
      if (this.d.over) finishTour();
      else startBattle(4, 0, () => { this.resume(); if (Engine.json('pt_tour_info').over) finishTour(); });
    }
  }
}

function finishTour() {
  const p = Engine.json('pt_tour_finish');
  saveSoon();
  const lines = [p.champion ? L('tourChamp') : p.finalist ? L('tourFinal') : L('tourOut')];
  if (p.perfect) lines.push(L('perfect'));
  if (p.money) lines.push(L('gotMoney', p.money.toLocaleString('en-US')));
  if (p.bp) lines.push(L('gotBP', p.bp));
  if (p.prize) lines.push(L('dailyPrize', p.prize));
  if (p.weekly) lines.push(L('weekly', p.weekly));
  while (topScreen() instanceof TourRunScreen) pop();
  message(lines, () => checkPending(() => topScreen().resume && topScreen().resume()));
}

// ---------------------------------------------------------------- pokedex --

function dexScreen() {
  const d = Engine.call('pt_dex');
  const items = [];
  for (let i = 0; i < 151; i++) {
    const st = d[i];
    items.push({ label: String(i + 1).padStart(3, '0') + ' ' + (st !== '0' ? JSON.parse(Engine.call('pt_species_name', i + 1)) : '----------'),
      right: st === '2' ? '●' : st === '1' ? '○' : '', icon: st !== '0' ? i + 1 : 0, rightColor: '#e8503a' });
  }
  return new ListScreen(L('pokedex'), items, { rowH: 24, right: () => S.caught + '/151' });
}

// ---------------------------------------------------------------- ajustes --

function settingsScreen() {
  const build = () => [
    { label: L('language'), right: lang === LANG_ES ? 'Español' : 'English', action: () => { setLang(lang === LANG_ES ? LANG_EN : LANG_ES); saveSoon(); topScreen().resume(); } },
    { label: L('invertAim') + ' X', right: prefs.invertX ? L('on') : L('off'), action: () => { prefs.invertX = !prefs.invertX; savePrefs(); topScreen().resume(); } },
    { label: L('invertAim') + ' Y', right: prefs.invertY ? L('on') : L('off'), action: () => { prefs.invertY = !prefs.invertY; savePrefs(); topScreen().resume(); } },
    { label: L('credits'), action: () => message(L('creditsText')) },
    { label: L('resetGame'), action: () => ask(L('resetQ'), [L('no'), L('yes')], k => {
      if (k === 1) { store.remove(SAVE_KEY).then(() => location.reload()); }
    }) },
  ];
  const s = new ListScreen(L('settings'), build(), { refresh: build });
  s.title = L('settings');
  return s;
}

// ------------------------------------------ aprender y evolucionar (cola) --

function checkPending(then) {
  const p = Engine.json('pt_pending');
  if (p.kind === 'none') { then(); return; }
  const next = () => { saveSoon(); checkPending(then); };
  if (p.kind === 'learned') message(L('learned', p.name, p.move), next);
  else if (p.kind === 'known') next();
  else if (p.kind === 'learn') {
    ask(L('wantsLearn', p.name, p.move), p.moves.map(m => m.name).concat([L('dontLearn')]), k => {
      const ok = Engine.call('pt_pending_resolve', k < 4 ? k : 4);
      if (ok) message(L('learned', p.name, p.move), next); else next();
    });
  } else if (p.kind === 'evolve') {
    message(L('evolving', p.name), () => ask(L('evolveQ'), [L('yes'), L('no')], k => {
      const ok = Engine.call('pt_pending_resolve', k === 0 ? 1 : 0);
      message(ok ? L('evolved', p.name, p.toName) : L('stopped', p.name), next);
    }));
  } else then();
}
