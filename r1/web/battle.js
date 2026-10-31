'use strict';
// Pantalla de combate. El motor resuelve cada turno (pt_battle_act) y devuelve
// los eventos en orden; aqui se reproducen uno a uno: texto, ataque, golpe y
// la barra de PS bajando cuando el golpe llega (no antes).

// fondo de cada combate: el de la ruta del encuentro, el gimnasio con su color, la Liga o el estadio
let encounterBg = 'grass';
const ROUTE_BG = ['grass', 'forest', 'cave', 'cave', 'grass', 'water', 'gym', 'cave', 'cave'];

function startBattle(kind, arg, after) {
  refresh();
  const v = Engine.json('pt_battle_start', kind, arg);
  if (v.error) { message(L('noEffect')); return; }
  let bg = encounterBg, accent;
  if (kind === 2) { bg = 'gym'; accent = shade(TYPE_COLORS[Engine.json('pt_gyms').gyms[arg].type], 30); }
  else if (kind === 3) bg = 'league';
  else if (kind === 4) bg = 'stadium';
  else if (encounterBg === 'gym') accent = '#a8a090';  // Central Electrica
  push(new BattleScreen(v, after, bg, accent));
}

const spNames = {};
function spName(sp) {
  if (!(sp in spNames) || spNames[sp]._lang !== lang) spNames[sp] = { n: JSON.parse(Engine.call('pt_species_name', sp)), _lang: lang };
  return spNames[sp].n;
}

class BattleScreen {
  constructor(v, after, bg = 'grass', accent) {
    this.v = v;
    this.after = after;
    this.bg = bg;
    this.accent = accent;
    this.t0 = performance.now();
    this.wild = v.kind === 'wild' || v.kind === 'legend';
    this.foeName = v.foeName || '';
    const me = v.me.party[v.me.active];
    this.disp = [this.monDisp(me), this.monDisp(v.foe.mon)];
    v.me.party.forEach(m => spriteImage(m.sp));
    spriteImage(v.foe.mon.sp);
    this.queue = [];
    this.cur = null;
    this.mode = 'queue';
    this.sel = 0;
    this.msel = 0;
    this.text = '';
    this.ended = false;
    // presentacion
    if (this.wild) this.say(v.kind === 'legend' ? L('legendAppeared', v.foe.mon.name) : L('wildAppeared', v.foe.mon.name));
    else {
      this.say(L('trainerWants', this.foeName));
      this.say(lang === LANG_ES ? `¡${this.foeName} sacó a ${v.foe.mon.name}!` : `${this.foeName} sent out ${v.foe.mon.name}!`);
    }
    this.say(lang === LANG_ES ? `¡Adelante, ${me.name}!` : `Go! ${me.name}!`);
  }

  monDisp(m) {
    return { sp: m.sp, name: m.name, lv: m.lv, hp: m.hp, max: m.max, st: m.st, hd: m.hd, shown: m.hp,
             anim: 'Idle', animT: 0, flashT: 0, faintT: 0, enterT: performance.now() };
  }

  // ---- cola de pasos: { text, dur, fn } ----
  say(text, dur = 1100, fn) { this.queue.push({ text, dur, fn }); }
  step(dur, fn) { this.queue.push({ text: null, dur, fn }); }

  runQueue(t) {
    if (this.cur && t - this.cur.t0 < this.cur.dur) return;
    this.cur = this.queue.shift() || null;
    if (!this.cur) { this.afterQueue(); return; }
    this.cur.t0 = t;
    if (this.cur.text) this.text = this.cur.text;
    if (this.cur.fn) this.cur.fn(t);
  }

  afterQueue() {
    const v = this.v;
    if (this.ended) return;
    if (v.outcome !== 0) { this.endBattle(); return; }
    if (v.needSwitch) { this.forcedSwitch(); return; }
    this.mode = 'menu';
    this.text = L('whatWill', this.disp[0].name);
  }

  // eventos del motor -> pasos animados
  play(res) {
    const ctx2 = { wild: this.wild, foeName: this.foeName };
    for (const ev of res.ev || []) {
      const d = () => this.disp[ev.s];
      if (ev.k === 'move') {
        this.say(eventText(ev, ctx2), 900, t => { d().anim = 'Attack'; d().animT = t; });
      } else if (ev.k === 'dmg') {
        if (ev.a) continue;  // al sustituto
        this.step(650, t => { const m = d(); m.from = m.shown; m.hp = ev.hp; m.hpT = t; m.flashT = t; m.anim = 'Hurt'; m.animT = t; });
      } else if (ev.k === 'heal' || ev.k === 'useitem') {
        const side = ev.k === 'useitem' ? 0 : ev.s;
        this.say(eventText(ev, ctx2), 1000, t => {
          const m = this.disp[side];
          if (ev.k === 'useitem' && ev.i !== this.v.me.active) return;
          m.from = m.shown; m.hp = ev.hp; m.max = ev.max; m.hpT = t;
        });
      } else if (ev.k === 'faint') {
        this.say(eventText(ev, ctx2), 1200, t => { d().faintT = t; });
      } else if (ev.k === 'switch') {
        this.say(eventText(ev, ctx2), 1000, () => { this.disp[ev.s] = this.monDisp({ ...ev.mon, hp: ev.hp }); });
      } else if (ev.k === 'status') {
        this.say(eventText(ev, ctx2), 1000, () => { if (ev.a < 0x80) d().st = ev.a; });
      } else if (ev.k === 'lvl') {
        this.say(eventText(ev, ctx2), 1100, () => { if (ev.i === this.v.me.active) this.disp[0].lv = ev.lv; });
      } else if (ev.k === 'ball') {
        // lo cuenta la pantalla del lanzamiento
      } else {
        const s = eventText(ev, ctx2);
        if (s) this.say(s, 1000);
      }
    }
    if (res.view) {
      const nv = res.view;
      this.step(1, () => {
        this.v = nv;
        // estados que cambian sin evento propio (curas, sueno que termina)
        this.disp[0].st = nv.me.party[nv.me.active].st;
        this.disp[1].st = nv.foe.mon.st;
      });
    }
    this.mode = 'queue';
  }

  act(kind, a = 0, b = 0, c = 0) {
    const res = Engine.json('pt_battle_act', kind, a, b, c);
    if (res.error) {
      const why = { move: L('noPP'), run: L('cantRun'), item: L('noEffect'), switch: L('alreadyOut') }[res.error];
      if (why) { this.say(why, 1000); this.mode = 'queue'; }
      return res;
    }
    this.play(res);
    return res;
  }

  forcedSwitch() {
    this.mode = 'wait';
    push(this.partyPicker(true));
  }

  partyPicker(forced) {
    const v = this.v;
    const items = v.me.party.map((m, i) => ({
      label: m.name, right: 'Lv' + m.lv, icon: m.sp, sub: m.hp ? `${m.hp}/${m.max}` : L('fainted'), hp: m.hp / m.max,
      disabled: !m.hp || i === v.me.active, why: !m.hp ? L('fainted') : L('alreadyOut'),
      action: () => { pop(); this.act(1, i); },
    }));
    return new ListScreen(forced ? L('chooseNext') : L('pokemon'), items, { rowH: 26, onBack: forced ? () => {} : () => pop() });
  }

  bagMenu() {
    refresh();
    const v = this.v;
    const items = [];
    if (v.canThrow) for (const id of BALLS) {
      const b = S.bag.find(x => x.id === id);
      if (b) items.push({ label: b.name, right: '×' + b.n, action: () => { pop(); this.throwBall(id); } });
    }
    for (const b of S.bag.filter(x => x.pocket === PK.MEDICINE)) {
      items.push({ label: b.name, right: '×' + b.n, action: () => {
        if (v.itemsLeft <= 0) { message(L('noItemsLeft')); return; }
        pop();
        const party = this.v.me.party.map((m, i) => ({ label: m.name, right: 'Lv' + m.lv, icon: m.sp, sub: `${m.hp}/${m.max}`, hp: m.hp / m.max,
          action: () => { pop(); const r = this.act(2, b.id, i); if (r.error) message(L('noEffect')); } }));
        push(new ListScreen(L('onWhich'), party, { rowH: 26 }));
      } });
    }
    push(new ListScreen(L('bag'), items, {}));
  }

  throwBall(ball) {
    const foe = this.v.foe.mon;
    let res = null;
    push(new ThrowScreen({
      sp: foe.sp, hd: foe.hd, ball,
      resolve: (hit, x10) => {
        res = Engine.json('pt_battle_act', 4, ball, x10, hit ? 1 : 0);
        if (res.error) return { res: 'noball' };
        const b = (res.ev || []).find(e => e.k === 'ball') || {};
        return { res: !b.hit ? 'miss' : b.caught ? 'caught' : 'broke', shakes: b.shakes || 0 };
      },
      done: r => {
        pop();
        saveSoon();
        if (!res || res.error) return;
        if (r.res === 'caught') { this.v = res.view; this.disp[1].faintT = performance.now(); this.mode = 'queue'; this.cur = null; this.endBattle(); return; }
        this.say(r.res === 'miss' ? L('missed') : L('brokeFree'), 1000);
        this.play(res);
      },
    }));
  }

  endBattle() {
    if (this.ended) return;
    this.ended = true;
    const v = this.v;
    if (v.outcome === 1 && !this.wild) this.say(L('youWon'), 1100);
    if (v.outcome === 2) this.say(L('youLost'), 1300);
    if (v.outcome === 9) this.say(L('caught', v.foe.mon.name), 1300);
    const r = Engine.json('pt_battle_end');
    saveSoon();
    if (r.money) this.say(L('gotMoney', r.money.toLocaleString('en-US')), 1100);
    if (r.badge) this.say(L('gotBadge', this.foeName), 1400);
    for (const tm of r.tms || []) this.say(L('gotTM', String(tm).padStart(2, '0')), 1100);
    if (r.prizeItem) this.say(L('gotPrize', r.prizeItem, r.prizeN), 1100);
    if (r.champion) this.say(L('champion'), 2000);
    if (r.blackout) this.say(r.lostMoney ? L('lostMoney', r.lostMoney) : L('blackoutNoMoney'), 1500);
    if (r.caught === '') this.say(L('boxFull'), 1200);
    this.step(1, () => {
      this.mode = 'done';
      pop();
      checkPending(() => { if (this.after) this.after(r); });
    });
    this.mode = 'queue';
    this.cur = null;
  }

  // ---- dibujo (al estilo de FireRed / LeafGreen) ----
  drawMonAt(i, x, y, side, t, k) {
    const m = this.disp[i];
    if (m.faintT && t - m.faintT > 400) return;
    const enter = Math.min(1, (t - m.enterT) / 350);
    let ox = 0, oy = 0;
    if (m.anim === 'Attack' && t - m.animT < 400) ox += Math.sin((t - m.animT) / 400 * Math.PI) * (i === 0 ? 10 : -10);
    if (m.faintT) oy = Math.min(1, (t - m.faintT) / 400) * 30;
    const flash = m.flashT && t - m.flashT < 450 ? (Math.floor((t - m.flashT) / 75) % 2 ? 0.8 : 0) : 0;
    let anim = 'Idle';
    if (m.anim !== 'Idle' && t - m.animT < 600) anim = m.anim;
    const s = monScale(m.sp, m.hd, side, k) * (0.3 + 0.7 * enter);
    ctx.save();
    if (m.faintT) { ctx.beginPath(); ctx.rect(0, 0, W, y + 1); ctx.clip(); }
    drawMon(m.sp, side, x + ox, y + oy, t, { scale: s, flash: enter < 1 ? 1 - enter : flash, anim });
    ctx.restore();
    // destello de la Poke Ball al salir
    if (enter < 1) {
      ctx.fillStyle = `rgba(255,255,255,${0.8 * (1 - enter)})`;
      ctx.beginPath(); ctx.arc(x, y - 14, 24 * enter + 4, 0, Math.PI * 2); ctx.fill();
    }
  }

  infoBox(i, x, y, w, t, mine) {
    const m = this.disp[i];
    if (m.hpT) {
      const p = Math.min(1, (t - m.hpT) / 500);
      m.shown = Math.round(m.from + (m.hp - m.from) * p);
    } else m.shown = m.hp;
    const h = mine ? 44 : 32;
    roundBox(x, y, w, h, BOX.info);
    textS(m.name, x + 7, y + 6);
    textS('Lv' + m.lv, x + w - 7, y + 6, INK, 'right');
    // barra de PS con la etiqueta "HP" de los juegos
    const by = y + 18;
    rect(x + 22, by - 1, w - 30, 7, '#404848');
    tinyText('HP', x + 24, by, '#f8c040');
    const f = m.shown / m.max, bw = w - 42;
    rect(x + 33, by, bw, 5, '#506058');
    rect(x + 33, by, Math.round(bw * Math.max(0, f)), 5, f > 0.5 ? '#58d080' : f > 0.2 ? '#f8c800' : '#f85838');
    rect(x + 33, by, Math.round(bw * Math.max(0, f)), 1, f > 0.5 ? '#a0f8c0' : f > 0.2 ? '#f8e888' : '#f8a888');
    statusChip(m.st, x + 7, by + 8);
    if (mine) {
      textS(`${m.shown}/${m.max}`, x + w - 7, by + 9, INK, 'right');
      // barra de EXP (del Pokemon real; en torneos no se muestra)
      const p = S && this.v.kind !== 'tour' ? S.party[this.v.me.active] : null;
      const ef = p && p.sp === m.sp ? (p.exp - p.expLv) / Math.max(1, p.expNext - p.expLv) : 0;
      rect(x + 22, y + h - 6, w - 30, 3, '#404848');
      rect(x + 23, y + h - 5, Math.round((w - 32) * Math.max(0, Math.min(1, ef))), 1, '#40a8f8');
    }
  }

  draw(t) {
    if (this.mode === 'queue') this.runQueue(t);
    const night = S && S.night;
    const FH = 176;
    drawBattleBg(this.bg, t, night, FH, this.accent);
    // las plataformas entran deslizandose
    const slide = Math.max(0, 1 - (t - this.t0) / 600);
    platform(176 + slide * 120, 88, 50, 11, this.bg, night);
    platform(64 - slide * 120, 166, 60, 12, this.bg, night);
    if (slide === 0) {
      this.drawMonAt(1, 176, 88, 'front', t, 38);
      this.drawMonAt(0, 64, 166, 'back', t, 44);
      this.infoBox(1, 4, 8, 124, t, false);
      this.infoBox(0, 112, 118, 124, t, true);
      if (this.v.shiny && Math.floor(t / 300) % 2) textS('✦', 206, 36, '#f8d030', 'left', '#806010');
    }
    // panel de abajo: cuadro oscuro de texto y, en el menu, recuadro blanco a la derecha
    roundBox(0, FH, W, H - FH, BOX.dark);
    if (this.mode === 'menu') this.drawMenu(t);
    else if (this.mode === 'moves') this.drawMoves(t);
    else {
      roundBox(4, FH + 6, W - 8, H - FH - 10, BOX.light);
      wrap(this.text, 25).slice(0, 5).forEach((l, i) => textS(l, 12, FH + 15 + i * 15));
      if (this.cur && this.cur.text && Math.floor(t / 400) % 2) {
        ctx.fillStyle = '#c03028';
        for (let i = 0; i < 4; i++) ctx.fillRect(W - 20 + i, H - 16 + i, 7 - i * 2, 1);
      }
    }
    battleWipe(this.t0, t);
  }

  drawMenu() {
    // texto a la izquierda en el cuadro oscuro, como en los juegos
    const lines = wrap(this.text, 25);
    lines.slice(0, 2).forEach((l, i) => textS(l, 10, 184 + i * 12, '#f8f8f8', 'left', '#182030'));
    roundBox(4, 210, W - 8, 68, BOX.light);
    const opts = [L('fight'), L('bagUp'), L('pkmnUp'), L('runUp')];
    opts.forEach((o, i) => {
      const x = 22 + (i % 2) * 112, y = 222 + Math.floor(i / 2) * 28;
      const off = i === 3 && !this.v.canRun;
      if (i === this.sel) cursor(x - 10, y, INK);
      textS(o, x, y, off ? INK_DIM : INK);
    });
  }

  drawMoves() {
    const me = this.v.me.party[this.v.me.active];
    roundBox(4, 182, W - 8, 62, BOX.light);
    me.moves.forEach((mv, i) => {
      const x = 20 + (i % 2) * 112, y = 192 + Math.floor(i / 2) * 26;
      if (i === this.msel) cursor(x - 10, y, INK);
      let name = mv.name;
      setFont(8);
      while (ctx.measureText(name).width > 96 && name.length > 3) name = name.slice(0, -1);
      textS(name, x, y, mv.pp ? INK : INK_DIM);
    });
    // PP y tipo del movimiento elegido
    const mv = me.moves[this.msel];
    roundBox(4, 246, W - 8, 32, BOX.light);
    if (mv) {
      textS('PP', 12, 252, INK);
      textS(`${mv.pp}/${mv.max}`, 60, 252, mv.pp ? INK : '#c03028');
      typeChip(mv.type, 110, 251);
      textS(mv.pow ? String(mv.pow) : '—', W - 12, 252, INK_DIM, 'right');
    }
  }

  input(ev) {
    if (this.mode === 'queue') {
      if ((ev.k === 'ok' || ev.k === 'tap') && this.cur && this.cur.text) this.cur.dur = 0;
      return;
    }
    if (this.mode === 'menu') {
      if (ev.k === 'up' || ev.k === 'left') this.sel = (this.sel + 3) % 4;
      else if (ev.k === 'down' || ev.k === 'right') this.sel = (this.sel + 1) % 4;
      else if (ev.k === 'tap' && ev.y > 210) {
        const i = (ev.x > W / 2 ? 1 : 0) + (ev.y > 242 ? 2 : 0);
        if (i === this.sel) this.pickMenu(); else this.sel = i;
      } else if (ev.k === 'ok') this.pickMenu();
      return;
    }
    if (this.mode === 'moves') {
      const n = this.v.me.party[this.v.me.active].moves.length;
      if (ev.k === 'up' || ev.k === 'left') this.msel = (this.msel + n - 1) % n;
      else if (ev.k === 'down' || ev.k === 'right') this.msel = (this.msel + 1) % n;
      else if (ev.k === 'back') this.mode = 'menu';
      else if (ev.k === 'tap') {
        if (ev.y < 182 || ev.y > 246) { this.mode = 'menu'; return; }
        const i = (ev.x > W / 2 ? 1 : 0) + (ev.y > 212 ? 2 : 0);
        if (i >= 0 && i < n) { if (i === this.msel) this.pickMove(); else this.msel = i; }
      } else if (ev.k === 'ok') this.pickMove();
    }
  }

  pickMenu() {
    if (this.sel === 0) {
      const me = this.v.me.party[this.v.me.active];
      if (!me.moves.some(m => m.pp > 0)) { this.act(0, 0); return; }  // Forcejeo
      this.mode = 'moves';
      this.msel = Math.min(this.msel, me.moves.length - 1);
    } else if (this.sel === 1) this.bagMenu();
    else if (this.sel === 2) push(this.partyPicker(false));
    else if (this.sel === 3) { if (this.v.canRun) this.act(3); else { this.say(L('cantRun'), 1000); this.mode = 'queue'; } }
  }

  pickMove() {
    const mv = this.v.me.party[this.v.me.active].moves[this.msel];
    if (!mv || !mv.pp) { this.say(L('noPP'), 900); this.mode = 'queue'; return; }
    this.act(0, this.msel);
  }

  resume() { if (this.mode === 'wait') this.mode = 'queue'; }
}
