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
    { label: 'Restore / Restaurar', action: () => restoreFlow() },
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

// el inicio (HomeScreen) es el mapa de Kanto: ver kanto.js

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
      { label: L('center'), right: healing, action: () => pokemonCenter(() => s.resume()) },
      ...(prefs.testMode ? [{ label: L('testSteps'), action: () => { walk(500); s.resume(); } }] : []),
    ];
  };
  const s = new ListScreen(L('explore'), build(), {
    y0: 62, refresh: build,
    drawTop: () => {
      roundBox(3, 21, W - 6, 34, BOX.light);
      textS('EE ' + S.ee + '/' + S.eeMax, 10, 27);
      bar(10, 37, W - 20, 4, S.ee / S.eeMax, theme.accent);
      text(S.outside ? (lang === LANG_ES ? `Evento en ${S.toEvent} pasos` : `Event in ${S.toEvent} steps`) : (S.steps + ' ' + (lang === LANG_ES ? 'pasos' : 'steps')), 10, 44, INK_DIM);
    },
  });
  return s;
}

// Centro Pokemon (reglas): en casa cura al momento; fuera de casa, en 30 minutos.
// El R1 no sabe si esta en el Wi-Fi de casa, asi que se le pregunta al jugador.
function pokemonCenter(then) {
  refresh();
  if (S.leagueActive) { message(L('noCenterLeague'), then); return; }
  const hurt = S.party.some(m => m.hp < m.max || m.st);
  if (!hurt) { message(L('healNone'), then); return; }
  const healNow = () => {
    Engine.call('pt_set_outside', 0);
    Engine.call('pt_heal', 1);
    saveSoon();
    message(L('healHome'), then);
  };
  if (!S.outside) { healNow(); return; }
  const left = S.healAt ? fmtTime(S.healAt - nowSec()) : '';
  const q = left ? L('healWaiting', left) : L('healAsk');
  ask(q, [L('healHomeNow'), left ? L('back') : L('heal30'), ...(left ? [] : [L('back')])], k => {
    if (k === 0) healNow();
    else if (k === 1 && !left) { Engine.call('pt_heal', 0); saveSoon(); message(L('healAway'), then); }
    else if (then) then();
  });
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
  Tele.track('event', e.kind, e.wild ? { sp: e.wild.sp, lv: e.wild.lv } : e.clsId !== undefined ? { cls: e.clsId } : null);
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
    refresh();
    message(ok ? L('bought') : S.money < o.price ? L('shopErr')[2] : L('shopErr')[5]);
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
    if (e.clsId !== undefined) trainerImage(TRAINER_KEYS[e.clsId]);
    (e.party || []).forEach(m => spriteImage(m.sp));
    const trainer = e.kind === 'trainer';
    const opts = trainer ? [L('battleIt'), L('decline')] : [L('throwBall'), L('battleIt'), L('run')];
    this.menu = opts;
    this.sel = 0;
  }
  draw(t) {
    const e = this.e;
    if (e.expires && nowSec() >= e.expires && topScreen() === this && !this.gone) {
      this.gone = true;
      Engine.call('pt_leave_encounter');
      replace(new MessageScreen(L('wildGone', e.wild.name)));
    }
    drawBattleBg(encounterBg, t, S.night, 196);
    platform(120, 146, 64, 12, encounterBg, S.night);
    if (e.kind === 'trainer') {
      const key = TRAINER_KEYS[e.clsId];
      if (!drawTrainer(key, 120, 150)) e.party.forEach((m, i) => drawMon(m.sp, 'front', 60 + i * 60, 146, t, { scale: monScale(m.sp, m.hd || 10, 'front', 34) }));
      roundBox(30, 8, W - 60, 22, BOX.info);
      textS(e.cls, W / 2, 15, INK, 'center');
      // sus Pokemon como Poke Balls, como en el juego
      e.party.forEach((m, i) => { ctx.save(); ctx.translate(W - 20 - i * 16, 44); ctx.scale(0.6, 0.6); drawBall(0, 0, 0, 0); ctx.restore(); });
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
  if (S.party.length >= 6 && S.boxCount >= 60) { message(L('pcFull')); return; }
  const balls = BALLS.map(id => S.bag.find(b => b.id === id)).filter(Boolean);
  if (!balls.length) { message(L('noBalls')); return; }
  if (balls.length === 1) { then(balls[0].id); return; }
  push(new ListScreen(L('chooseBall'), balls.map(b => ({ label: b.name, right: '×' + b.n, action: () => { pop(); then(b.id); } }))));
}

// ------------------------------------------------------------ lanzamiento --
// Sencillo: el circulo se achica alrededor del Pokemon y se lanza tocando la
// pantalla, girando la rueda o con el boton. La bola siempre llega; mientras mas
// chico el circulo, mejor el bono (Nice x1,3, Great x1,6, Excellent x2).

const BALL_HOME = { x: W / 2, y: 228 };
const TARGET = { x: W / 2, y: 112 };

class ThrowScreen {
  constructor(o) {
    this.o = o;
    this.phase = 'ready';
    this.t0 = performance.now();
    this.label = '';
    input.wheelUps = [];
    spriteImage(o.sp);
  }
  ring(t) {
    const p = ((t - this.t0) % 1400) / 1400;
    return 40 - 30 * p;  // de 40 a 10 px
  }
  ringColor(r) {
    const f = (r - 10) / 30;
    return f < 0.2 ? '#f85838' : f < 0.45 ? '#f8a030' : f < 0.7 ? '#f8d030' : '#58d080';
  }
  draw(t) {
    const night = S.night;
    drawBattleBg(encounterBg, t, night, 248);
    platform(TARGET.x, TARGET.y + 24, 60, 11, encounterBg, night);
    const scale = monScale(this.o.sp, this.o.hd, 'front', 52);
    const ph = this.phase, dt = t - (this.phaseT || 0);

    // el Pokemon: visible salvo dentro de la bola
    let monAlpha = 1, monScaleK = 1, flash = 0;
    if (ph === 'open') { monScaleK = Math.max(0, 1 - dt / 350); flash = 1; }
    if (ph === 'drop' || ph === 'shake' || (ph === 'result' && this.result.res === 'caught')) monAlpha = 0;
    if (ph === 'result' && this.result.res !== 'caught') { monScaleK = Math.min(1, dt / 300); flash = Math.max(0, 1 - dt / 400); }
    if (monAlpha && monScaleK > 0.02) {
      drawMon(this.o.sp, 'front', TARGET.x, TARGET.y + 24, t, { scale: scale * monScaleK, flash });
    }
    if (ph === 'ready') {
      const r = this.ring(t);
      ctx.strokeStyle = this.ringColor(r);
      ctx.lineWidth = 3;
      ctx.beginPath(); ctx.arc(TARGET.x, TARGET.y + 6, r, 0, Math.PI * 2); ctx.stroke();
      ctx.strokeStyle = 'rgba(255,255,255,0.7)';
      ctx.lineWidth = 1;
      ctx.beginPath(); ctx.arc(TARGET.x, TARGET.y + 6, 40, 0, Math.PI * 2); ctx.stroke();
      // la bola espera abajo, moviendose un poco
      drawBall(BALL_HOME.x, BALL_HOME.y + Math.sin(t / 250) * 2, this.o.ball, 0);
    }
    if (ph === 'fly') {
      const p = Math.min(1, dt / 520);
      const x = BALL_HOME.x + (TARGET.x - BALL_HOME.x) * p;
      const y = BALL_HOME.y + (TARGET.y - BALL_HOME.y) * p - Math.sin(p * Math.PI) * 70;
      drawBall(x, y, this.o.ball, p * 14);
      if (p >= 1) this.setPhase('open', t);
    }
    if (ph === 'open') {
      drawBall(TARGET.x, TARGET.y, this.o.ball, 0, true);
      ctx.fillStyle = `rgba(255,255,255,${Math.max(0, 0.9 - dt / 400)})`;
      ctx.beginPath(); ctx.arc(TARGET.x, TARGET.y + 8, 10 + dt / 12, 0, Math.PI * 2); ctx.fill();
      if (dt > 400) this.setPhase('drop', t);
    }
    if (ph === 'drop') {
      const p = Math.min(1, dt / 300);
      const y = TARGET.y + (TARGET.y + 20 - TARGET.y) * p - Math.sin(p * Math.PI) * 8;
      drawBall(TARGET.x, y, this.o.ball, 0);
      if (p >= 1) {
        this.result = this.o.resolve(true, this.x10);
        refresh();
        Tele.track('throw', this.result.res, { sp: this.o.sp, x10: this.x10, shakes: this.result.shakes });
        if (this.result.res === 'noball' || this.result.res === 'none') { this.phase = 'done'; this.o.done(this.result); return; }
        this.setPhase('shake', t);
      }
    }
    if (ph === 'shake') {
      const k = Math.floor(dt / 650);
      const local = dt % 650;
      const wob = k < this.result.shakes && local < 400 ? Math.sin(local / 400 * Math.PI * 2) * 0.5 : 0;
      drawBall(TARGET.x, TARGET.y + 20, this.o.ball, wob);
      if (k >= Math.max(1, this.result.shakes) + (this.result.res === 'caught' ? 0 : 0)) this.setPhase('result', t);
    }
    if (ph === 'result') {
      if (this.result.res === 'caught') {
        drawBall(TARGET.x, TARGET.y + 20, this.o.ball, 0);
        for (let i = 0; i < 3; i++) {
          const a = -Math.PI / 2 + (i - 1) * 0.7, d = 10 + Math.min(14, dt / 30);
          textS('★', TARGET.x + Math.cos(a) * d - 4, TARGET.y + 14 + Math.sin(a) * d, '#f8d030', 'left', '#806010');
        }
      }
      if (dt > 1300) { this.phase = 'done'; this.o.done(this.result); }
    }

    // panel
    roundBox(0, 248, W, 34, BOX.dark);
    const msg = ph === 'ready' ? L('throwHint') : ph === 'result' || ph === 'done' ? this.resultText()
              : (this.label ? this.label + '! ' : '') + '. . .'.slice(0, 1 + Math.floor(t / 300) % 3 * 2);
    wrap(msg, 27).slice(0, 2).forEach((l, i) => textS(l, 8, 254 + i * 12, '#f8f8f8', 'left', '#182030'));
    const b = S.bag.find(x => x.id === this.o.ball);
    if (ph === 'ready') {
      backButton();
      roundBox(W - 56, 2, 54, 20, BOX.info);
      ctx.save(); ctx.translate(W - 44, 12); ctx.scale(0.7, 0.7); drawBall(0, 0, this.o.ball, 0); ctx.restore();
      textS('×' + (b ? b.n : 0), W - 8, 8, INK, 'right');
    }
  }
  setPhase(p, t) { this.phase = p; this.phaseT = t; }
  resultText() {
    const r = this.result || {};
    const name = spName(this.o.sp);
    const pre = this.label ? this.label + '! ' : '';
    if (r.res === 'caught') return pre + L('caught', name);
    if (r.res === 'fled') return L('fled', name);
    if (r.res === 'broke') return pre + L('brokeFree');
    if (r.res === 'full') return L('boxFull');
    return '';
  }
  throwNow(t) {
    if (this.phase !== 'ready') return;
    const r = this.ring(t);
    const f = (r - 10) / 30;
    this.x10 = f < 0.2 ? 20 : f < 0.45 ? 16 : f < 0.7 ? 13 : 10;
    this.label = this.x10 === 20 ? L('throwNames')[3] : this.x10 === 16 ? L('throwNames')[2] : this.x10 === 13 ? L('throwNames')[1] : '';
    this.setPhase('fly', t);
  }
  input(ev) {
    if (this.phase !== 'ready') return;
    if (ev.k === 'back' || inBack(ev)) { pop(); return; }
    // tocar, el boton o cualquier paso de rueda lanzan
    if (ev.k === 'tap' || ev.k === 'ok' || ev.wheel) this.throwNow(performance.now());
  }
}

// Poke Ball de 9 px de radio; open = abierta (la mitad de arriba levantada)
function drawBall(x, y, ball, spin, open) {
  const colors = ['#e8503a', '#3a78d8', '#303040', '#a040c0'];
  const R = 9;
  ctx.save();
  ctx.translate(Math.round(x), Math.round(y));
  ctx.rotate(spin);
  ctx.fillStyle = 'rgba(0,0,0,0.25)';
  ctx.beginPath(); ctx.ellipse(1, R + 2, R, 3, 0, 0, Math.PI * 2); ctx.fill();
  ctx.fillStyle = '#f8f8f8';
  ctx.beginPath(); ctx.arc(0, 0, R, 0, Math.PI); ctx.fill();
  ctx.save();
  if (open) { ctx.translate(0, -5); ctx.rotate(-0.5); }
  ctx.fillStyle = colors[ball] || colors[0];
  ctx.beginPath(); ctx.arc(0, 0, R, Math.PI, 0); ctx.fill();
  if (ball === 2) { rect(-6, -6, 3, 4, '#f8d030'); rect(3, -6, 3, 4, '#f8d030'); }  // Ultra Ball
  if (ball === 1) { rect(-6, -5, 3, 2, '#e8503a'); rect(3, -5, 3, 2, '#e8503a'); }  // Great Ball
  ctx.restore();
  ctx.strokeStyle = '#282830';
  ctx.lineWidth = 1.5;
  ctx.beginPath(); ctx.arc(0, 0, R, 0, Math.PI * 2); ctx.stroke();
  rect(-R, -1, R * 2, 2, '#282830');
  ctx.fillStyle = '#282830';
  ctx.beginPath(); ctx.arc(0, 0, 3.5, 0, Math.PI * 2); ctx.fill();
  ctx.fillStyle = '#f8f8f8';
  ctx.beginPath(); ctx.arc(0, 0, 2, 0, Math.PI * 2); ctx.fill();
  rect(-5, -6, 2, 2, 'rgba(255,255,255,0.8)');
  ctx.restore();
}

// ----------------------------------------------------------------- equipo --

// mode: menu | switch | target (objeto) | teach (MT)
// equipo en tarjetas (PartyScreen, cards.js). mode: menu | swap | target | teach
function partyScreen(mode, opts = {}) {
  return new PartyScreen(Object.assign({ mode }, opts));
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
      if (Engine.call('pt_team_locked')) { message(L('teamLocked')); return; }
      if (S.boxCount >= 60) { message(L('pcFull')); return; }
      Engine.call('pt_deposit', i);
      saveSoon();
      topScreen().resume();
    } },
    { label: L('release'), action: () => {
      pop();
      if (m.partner) { message(L('partnerStays')); return; }
      if (S.party.length <= 1) { message(L('lastOne')); return; }
      releaseFlow(0, i, m.name);
    } },
    { label: L('pc') + ' (' + S.boxCount + ')', action: () => { pop(); push(boxScreen()); } },
  ];
  push(new ListScreen(m.name, items));
}

function releaseFlow(inBox, idx, name) {
  ask(L('releaseQ', name), [L('no'), L('yes')], k => {
    if (k !== 1) return;
    if (Engine.call('pt_release', inBox, idx)) { saveSoon(); message(L('released', name), () => topScreen().resume && topScreen().resume()); }
    else message(L('teamLocked'));
  });
}

// PC de 12 cajas de 20, como en gen 1: se pasa de caja con las filas de arriba y abajo
function boxScreen(startPage = 0) {
  let page = startPage, pages = 12;
  const s = new ListScreen('', [], { rowH: 24, refresh: () => build(), right: () => S.boxCount + '/' + (S.boxMax || 240) });
  const build = () => {
    refresh();
    const d = Engine.json('pt_box_page', page);
    pages = d.pages;
    s.title = L('box') + ' ' + (page + 1) + '/' + pages;
    const items = [{ label: '◀ ' + L('box') + ' ' + ((page + pages - 1) % pages + 1), action: () => { page = (page + pages - 1) % pages; s.sel = 0; s.resume(); } }];
    d.mons.forEach((m, i) => {
      const g = page * 20 + i;
      items.push({
        label: m.name + (m.shiny ? ' ✦' : ''), right: 'Lv' + m.lv, icon: m.sp, sub: `${m.hp}/${m.max}`,
        action: () => ask(m.name, [L('withdraw'), L('summary'), L('release'), L('back')], k => {
          if (k === 0) {
            if (S.party.length >= 6) { message(L('partyFull')); return; }
            if (Engine.call('pt_team_locked')) { message(L('teamLocked')); return; }
            Engine.call('pt_box_withdraw', g);
            saveSoon();
            s.resume();
          } else if (k === 1) push(new SummaryScreen(2, g));
          else if (k === 2) ask(L('releaseQ', m.name), [L('no'), L('yes')], r => {
            if (r !== 1) return;
            if (Engine.call('pt_box_release', g)) { saveSoon(); message(L('released', m.name), () => s.resume()); }
            else message(L('teamLocked'));
          });
        }),
      });
    });
    if (!d.mons.length) items.push({ label: L('emptyBox'), disabled: true });
    items.push({ label: L('box') + ' ' + ((page + 1) % pages + 1) + ' ▶', action: () => { page = (page + 1) % pages; s.sel = 0; s.resume(); } });
    return items;
  };
  s.items = s.withBack(build());
  return s;
}

// la ficha del Pokemon (SummaryScreen) esta en cards.js

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
    if (it.isBack) return;
    const p = pockets[k];
    Object.defineProperty(it, 'right', { get: () => p === 'tm' ? S.tms.length : S.bag.filter(b => b.pocket === p).length });
  });
  return s;
}

function pocketScreen(pocket, title, opts) {
  const build = () => {
    refresh();
    if (pocket === 'tm') {
      return S.tms.map(tm => ({ label: (tm.num > 50 ? L('hm') + String(tm.num - 50).padStart(2, '0') : L('tm') + String(tm.num).padStart(2, '0')) + ' ' + tm.name,
        right: tm.num > 50 ? '' : '×' + tm.n, desc: moveDesc(tm), action: () => teachFlow(tm) }));
    }
    return S.bag.filter(b => b.pocket === pocket).map(b => ({ label: b.name, right: '×' + b.n, desc: itemDesc(b.id), action: () => useFlow(b, opts) }));
  };
  return new ListScreen(title, build(), { refresh: build, descBox: true });
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
        return { label: (it.tm ? it.label + ' ' : '') + it.name, right: it.bp + ' BP', desc: it.tm ? '' : itemDesc(it.id), action: () => {
          const r = Engine.call('pt_buy', 3, it.idx, 1);
          saveSoon();
          message(r === 0 ? L('bought') : L('shopErr')[r]);
        } };
      }
      return { label: it.name, right: money(it.price), desc: itemDesc(it.id) + (it.have ? '  (' + L('have') + ' ' + it.have + ')' : ''), action: () => {
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
      for (const tm of d.tms) items.push({ label: L('tm') + String(tm.num).padStart(2, '0') + ' ' + tm.name, right: money(tm.price), action: () => {
        const r = Engine.call('pt_buy', 4, tm.slot, 1);
        saveSoon();
        message(r === 0 ? L('bought') : L('shopErr')[r]);
      } });
    }
    return items;
  };
  return new ListScreen(title, build(), { refresh: build, descBox: true, right: () => which === 3 ? S.bp + ' BP' : money(S.money) });
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
      sub: gym.open ? '' : (i > 0 && !g.gyms[i - 1].badge ? L('needBadge')
        : gym.route >= 0 ? L('routeToOpen', Engine.json('pt_challenges').list[gym.route].name) : L('kmToOpen', gym.km - g.km)),
      disabled: !gym.open,
      action: () => ask(gym.leader + ' · ' + typeName(gym.type), [L('challenge'), L('back')], k => {
        if (k === 0) startBattle(2, i, () => topScreen().resume());
      }),
    }));
    items.push({
      label: L('league'), right: g.leagueActive ? (g.leagueIdx + 1) + '/5' : '', sub: g.leagueOpen ? (g.leagueActive ? L('leagueNext', g.elite[g.leagueIdx].name) : '') : L('leagueNeed'),
      disabled: !g.leagueOpen,
      action: () => {
        const opts = g.leagueActive ? [L('leagueNext', g.elite[g.leagueIdx].name), L('pokemon'), L('bag'), L('leagueQuit'), L('back')] : [L('challenge'), L('back')];
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
    return { label: t.name, right: money(fee), sub: `${t.champBp * (feat ? 2 : 1)} BP`, disabled: cup === 7 && k !== 3, why: L('masterOnly'),
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
    const sel = this.sel || 0;
    roundBox(20, 216, W - 40, 26, sel === 0 ? BOX.sel : BOX.light);
    if (sel === 0) cursor(30, 225, INK);
    textS(d.over ? L('continueTour') : L('battleIt'), W / 2 + 6, 225, INK, 'center');
    roundBox(20, 246, W - 40, 26, sel === 1 ? BOX.sel : BOX.light);
    if (sel === 1) cursor(30, 255, INK);
    textS(L('back'), W / 2 + 6, 255, INK, 'center');
  }
  input(ev) {
    if (ev.k === 'up' || ev.k === 'down') { this.sel = 1 - (this.sel || 0); return; }
    if (ev.k === 'back' || (ev.k === 'tap' && ev.y > 244) || (ev.k === 'ok' && this.sel === 1)) { pop(); return; }
    if (ev.k === 'ok' || (ev.k === 'tap' && ev.y > 212)) {
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
  const seen = [];
  for (let i = 0; i < 151; i++) if (d[i] !== '0') seen.push(i + 1);
  for (let i = 0; i < 151; i++) {
    const st = d[i];
    items.push({ label: String(i + 1).padStart(3, '0') + ' ' + (st !== '0' ? JSON.parse(Engine.call('pt_species_name', i + 1)) : '----------'),
      right: st === '2' ? '●' : st === '1' ? '○' : '', icon: st !== '0' ? i + 1 : 0, rightColor: '#e8503a',
      disabled: st === '0', why: L('notSeen'),
      action: () => push(new DexEntryScreen(i + 1, seen)) });
  }
  return new ListScreen(L('pokedex'), items, { rowH: 24, right: () => S.caught + '/151' });
}

// ---------------------------------------------------------------- ajustes --

function settingsScreen() {
  const build = () => [
    { label: L('language'), right: lang === LANG_ES ? 'Español' : 'English', action: () => { setLang(lang === LANG_ES ? LANG_EN : LANG_ES); saveSoon(); topScreen().resume(); } },
    { label: L('renamePlayer'), right: profile.player, action: () => push(new KeyboardScreen(L('yourName'), profile.player, n => { profile.player = n; saveProfile(); })) },
    { label: L('renameRival'), right: profile.rival, action: () => push(new KeyboardScreen(L('rivalName'), profile.rival, n => { profile.rival = n; saveProfile(); })) },
    { label: L('autoEvents'), right: prefs.autoEvents !== false ? L('on') : L('off'), action: () => { prefs.autoEvents = prefs.autoEvents === false; savePrefs(); topScreen().resume(); } },
    { label: L('testMode'), right: prefs.testMode ? L('on') : L('off'), action: () => { prefs.testMode = !prefs.testMode; savePrefs(); topScreen().resume(); } },
    { label: L('cloud'), right: prefs.cloudCode && !prefs.cloudOff ? L('on') : L('off'), action: () => push(cloudScreen()) },
    { label: L('telemetry'), right: prefs.telemetry !== false ? L('on') : L('off'), action: () => { prefs.telemetry = prefs.telemetry === false; savePrefs(); topScreen().resume(); } },
    { label: L('errorLog'), right: errLog.length || '', action: () => push(errorLogScreen()) },
    { label: L('credits'), action: () => message(L('creditsText')) },
    { label: L('resetGame'), action: () => ask(L('resetQ'), [L('no'), L('yes')], k => {
      if (k === 1) { resetting = true; Promise.all([store.remove(SAVE_KEY), store.remove(PROFILE_KEY)]).then(() => location.reload()); }
    }) },
  ];
  const s = new ListScreen(L('settings'), build(), { refresh: build });
  s.title = L('settings');
  return s;
}

// registro de errores guardado en el aparato (para revisar fallos en el R1)
function errorLogScreen() {
  const build = () => {
    const items = errLog.slice().reverse().map(e => ({
      label: e.t.slice(5, 16).replace('T', ' ') + ' ' + e.where, sub: e.msg,
      action: () => message([e.where, e.msg, e.screens || '', e.trace || '']),
    }));
    if (errLog.length) items.push({ label: L('clearLog'), action: () => { errLog = []; try { localStorage.removeItem(ERRLOG_KEY); } catch (x) { /* nada */ } topScreen().resume(); } });
    else items.push({ label: L('noErrors'), disabled: true });
    return items;
  };
  return new ListScreen(L('errorLog'), build(), { rowH: 26, refresh: build });
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
    // animacion de evolucion; volver la detiene
    const ev = new EvolutionScreen(p, ok => { Engine.call('pt_pending_resolve', ok ? 1 : 0); saveSoon(); });
    ev.then = next;
    push(ev);
  } else then();
}
