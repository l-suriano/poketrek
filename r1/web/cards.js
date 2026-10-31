'use strict';
// Fichas al estilo FireRed: equipo en tarjetas, ficha del Pokemon en 3
// paginas, Pokedex con ficha por especie, descripcion de cada objeto, y el
// aviso "!" cuando sale un encuentro mientras caminas afuera.

// -------------------------------------------------------------- objetos -----

const ITEM_DESC = [
  ['Una bola para atrapar Pokémon salvajes.', 'A ball for catching wild Pokémon.'],
  ['Atrapa mejor que una Poké Ball (x1.5).', 'Catches better than a Poké Ball (x1.5).'],
  ['Atrapa mucho mejor que una Poké Ball (x2).', 'Catches far better than a Poké Ball (x2).'],
  ['Atrapa a cualquier Pokémon sin fallar.', 'Catches any Pokémon without fail.'],
  ['Recupera 20 PS de un Pokémon.', 'Restores 20 HP to a Pokémon.'],
  ['Recupera 50 PS de un Pokémon.', 'Restores 50 HP to a Pokémon.'],
  ['Recupera 200 PS de un Pokémon.', 'Restores 200 HP to a Pokémon.'],
  ['Recupera todos los PS de un Pokémon.', 'Fully restores a Pokémon\'s HP.'],
  ['Recupera todos los PS y cura el estado.', 'Fully restores HP and cures status.'],
  ['Revive a un Pokémon debilitado con la mitad de sus PS.', 'Revives a fainted Pokémon with half its HP.'],
  ['Revive a un Pokémon debilitado con todos sus PS.', 'Revives a fainted Pokémon with full HP.'],
  ['Cura el envenenamiento.', 'Cures poison.'],
  ['Cura la parálisis.', 'Cures paralysis.'],
  ['Despierta a un Pokémon dormido.', 'Wakes up a sleeping Pokémon.'],
  ['Cura las quemaduras.', 'Heals a burn.'],
  ['Descongela a un Pokémon.', 'Thaws out a frozen Pokémon.'],
  ['Cura cualquier problema de estado.', 'Cures any status problem.'],
  ['Hace evolucionar a ciertos Pokémon: Vulpix, Growlithe, Eevee.', 'Evolves some Pokémon: Vulpix, Growlithe, Eevee.'],
  ['Hace evolucionar a ciertos Pokémon: Poliwhirl, Shellder, Staryu, Eevee.', 'Evolves some Pokémon: Poliwhirl, Shellder, Staryu, Eevee.'],
  ['Hace evolucionar a ciertos Pokémon: Pikachu, Eevee.', 'Evolves some Pokémon: Pikachu, Eevee.'],
  ['Hace evolucionar a ciertos Pokémon: Gloom, Weepinbell, Exeggcute.', 'Evolves some Pokémon: Gloom, Weepinbell, Exeggcute.'],
  ['Hace evolucionar a ciertos Pokémon: Nidorina, Nidorino, Clefairy, Jigglypuff.', 'Evolves some Pokémon: Nidorina, Nidorino, Clefairy, Jigglypuff.'],
  ['Sube los PS de un Pokémon.', 'Raises a Pokémon\'s HP.'],
  ['Sube el Ataque de un Pokémon.', 'Raises a Pokémon\'s Attack.'],
  ['Sube la Defensa de un Pokémon.', 'Raises a Pokémon\'s Defense.'],
  ['Sube la Velocidad de un Pokémon.', 'Raises a Pokémon\'s Speed.'],
  ['Sube el Especial de un Pokémon.', 'Raises a Pokémon\'s Special.'],
  ['Sube un nivel a un Pokémon.', 'Raises a Pokémon by one level.'],
  ['Una pepita de oro: en la tienda vale ₽5,000.', 'A gold nugget: sells for ₽5,000.'],
  ['Atrae un Pokémon cada 5 min durante 30 min.', 'Lures a Pokémon every 5 min for 30 min.'],
  ['Hace evolucionar a los que evolucionan por intercambio.', 'Evolves Pokémon that evolve by trade.'],
  ['Los del equipo que no luchan ganan la mitad de la EXP.', 'Party members that don\'t battle get half the EXP.'],
];
function itemDesc(id) { const d = ITEM_DESC[id]; return d ? d[lang === LANG_ES ? 0 : 1] : ''; }

// gen 1: la categoria depende del tipo (fisico o especial)
const PHYSICAL_TYPES = new Set([0, 1, 2, 3, 4, 5, 6, 7]);
function moveDesc(mv) {
  const es = lang === LANG_ES;
  const cat = mv.pow ? (PHYSICAL_TYPES.has(mv.type) ? (es ? 'Físico' : 'Physical') : (es ? 'Especial' : 'Special')) : (es ? 'Estado' : 'Status');
  return cat + (mv.pow ? ` · ${es ? 'Pod.' : 'Pow.'} ${mv.pow}` : '') + (mv.acc ? ` · ${es ? 'Prec.' : 'Acc.'} ${mv.acc}` : '');
}

// ----------------------------------------------------------- equipo ---------
// Tarjetas: la primera grande arriba, las demas debajo; "Volver" al final.

class PartyScreen {
  constructor(opts = {}) {
    this.opts = opts;
    this.sel = 0;
  }
  enter() { refresh(); S.party.forEach(m => spriteImage(m.sp)); }
  resume() { refresh(); this.sel = Math.min(this.sel, S.party.length); }
  disabled(i) { return this.opts.tm && !Engine.call('pt_can_learn_tm', this.opts.tm, i); }
  card(i, x, y, w, h, t, big) {
    const m = S.party[i];
    const on = i === this.sel;
    roundBox(x, y, w, h, on ? BOX.sel : m.hp ? BOX.light : { border: '#604040', rim: '#d8a8a8', fill: '#f8e8e8' });
    const s = Math.min(big ? 1.4 : 1, monScale(m.sp, m.hd, 'front', big ? 34 : 24));
    drawMon(m.sp, 'front', x + (big ? 28 : 20), y + h - 6, t, { scale: s, anim: on ? 'Idle' : 'Idle', alpha: this.disabled(i) ? 0.4 : 1 });
    const tx = x + (big ? 56 : 42);
    textS(fitText(m.name + (m.shiny ? '✦' : ''), w - (tx - x) - 46), tx, y + 6, this.disabled(i) ? INK_DIM : INK);
    textS('Lv' + m.lv, x + w - 8, y + 6, INK, 'right');
    if (m.partner) textS('♥', x + w - 46, y + 6, '#e05070');
    const by = y + (big ? 22 : 18);
    rect(tx, by, w - (tx - x) - 10, 6, '#404848');
    tinyText('HP', tx + 2, by + 1, '#f8c040');
    const bw = w - (tx - x) - 24, f = m.hp / m.max;
    rect(tx + 12, by + 1, bw, 4, '#506058');
    rect(tx + 12, by + 1, Math.round(bw * f), 4, f > 0.5 ? '#58d080' : f > 0.2 ? '#f8c800' : '#f85838');
    textS(`${m.hp}/${m.max}`, x + w - 8, by + 9, INK, 'right');
    statusChip(m.st, tx, by + 9);
    if (!m.hp && !m.st) textS(L('fainted').toUpperCase(), tx, by + 9, '#c03028');
  }
  draw(t) {
    menuBg();
    header(this.opts.title || L('pokemon'), S.party.length + '/6', true);
    if (S.party.length) this.card(0, 3, 22, W - 6, 52, t, true);
    for (let i = 1; i < S.party.length; i++) this.card(i, 3, 76 + (i - 1) * 36, W - 6, 34, t, false);
    const by = 76 + Math.max(0, S.party.length - 1) * 36 + 2;
    const back = this.sel === S.party.length;
    roundBox(3, Math.min(by, H - 22), W - 6, 20, back ? BOX.sel : BOX.light);
    if (back) cursor(10, Math.min(by, H - 22) + 6, INK);
    textS('◀ ' + L('back'), 20, Math.min(by, H - 22) + 6, INK_DIM);
    if (this.opts.hint) dialogBox([this.opts.hint], t, false);
  }
  input(ev) {
    const n = S.party.length + 1;
    if (ev.k === 'up' || ev.k === 'left') this.sel = (this.sel + n - 1) % n;
    else if (ev.k === 'down' || ev.k === 'right') this.sel = (this.sel + 1) % n;
    else if (ev.k === 'back' || inBack(ev)) pop();
    else if (ev.k === 'ok') this.choose(this.sel);
    else if (ev.k === 'tap') {
      let i = ev.y < 76 ? 0 : 1 + Math.floor((ev.y - 76) / 36);
      if (i > S.party.length) i = S.party.length;
      if (i === this.sel) this.choose(i); else this.sel = i;
    }
  }
  choose(i) {
    if (i >= S.party.length) { pop(); return; }
    if (this.disabled(i)) { message(L('cantLearn')); return; }
    const m = S.party[i];
    if (this.opts.pick) this.opts.pick(i, m);
    else monMenu(i, m);
  }
}

// --------------------------------------------------- ficha del Pokemon ------
// 3 paginas como en FireRed: datos, stats y movimientos. La rueda cambia de
// pagina; volver cierra.

class SummaryScreen {
  constructor(inBox, idx) {
    this.m = inBox === 2 ? Engine.json('pt_box_mon', idx) : Engine.json('pt_mon', inBox, idx);
    this.sp = Engine.json('pt_species', this.m.sp);
    this.page = 0;
  }
  portrait(t) {
    const m = this.m;
    roundBox(3, 22, 94, 84, BOX.light);
    ctx.save(); ctx.beginPath(); ctx.rect(6, 25, 88, 78); ctx.clip();
    ctx.translate(6, 25); ctx.scale(88 / W, 78 / 176); drawBattleBg('grass', t, S.night, 176); ctx.restore();
    platform(50, 96, 30, 6, 'grass', S.night);
    drawMon(m.sp, 'front', 50, 96, t, { scale: monScale(m.sp, m.hd, 'front', 36) });
    if (m.shiny) textS('✦', 86, 30, '#f8d030', 'right', '#806010');
  }
  draw(t) {
    const m = this.m, es = lang === LANG_ES;
    menuBg();
    const titles = es ? ['DATOS', 'STATS', 'MOVIMIENTOS'] : ['INFO', 'STATS', 'MOVES'];
    header(titles[this.page], (this.page + 1) + '/3', true);
    // puntos de pagina
    for (let i = 0; i < 3; i++) rect(W - 76 + i * 10, 8, 6, 4, i === this.page ? '#f8d030' : '#6890c0');
    this.portrait(t);
    roundBox(99, 22, 138, 84, BOX.light);
    textS(m.name, 106, 28);
    textS('Lv' + m.lv, 230, 28, INK, 'right');
    let x = typeChip(m.t1, 106, 42);
    if (m.t2 !== 255) typeChip(m.t2, 110 + x, 42);
    textS(`HP ${m.hp}/${m.max}`, 106, 58);
    bar(106, 68, 124, 4, m.hp / m.max);
    statusChip(m.st, 204, 56);
    // confianza en corazones (5 = 100)
    for (let i = 0; i < 5; i++) textS('♥', 106 + i * 12, 80, m.trust >= (i + 1) * 20 ? '#e05070' : '#c8c0c8');
    textS(L('trust'), 230, 80, INK_DIM, 'right');
    if (this.page === 0) this.pageInfo(es);
    else if (this.page === 1) this.pageStats(es);
    else this.pageMoves(es);
  }
  pageInfo(es) {
    const m = this.m, sp = this.sp || {};
    roundBox(3, 108, W - 6, 170, BOX.light);
    const rows = [
      [es ? 'No. Pokédex' : 'Pokédex No.', String(m.sp).padStart(3, '0')],
      [es ? 'Entrenador' : 'Trainer', profile.player + (m.partner ? (es ? ' ♥ Compañero' : ' ♥ Partner') : '')],
      [es ? 'Altura' : 'Height', ((sp.hd || 0) / 10).toFixed(1) + ' m'],
      [es ? 'Peso' : 'Weight', ((sp.wt || 0) / 10).toFixed(1) + ' kg'],
      ['EXP', String(m.exp)],
      [es ? 'Para Lv ' + (m.lv + 1) : 'To Lv ' + (m.lv + 1), m.lv < 100 ? String(m.expNext - m.exp) : '-'],
    ];
    rows.forEach(([k, v], i) => { textS(k, 11, 116 + i * 18, INK_DIM); textS(fitText(v, 120), 229, 116 + i * 18, INK, 'right'); });
    bar(11, 116 + 6 * 18, W - 22, 4, (m.exp - m.expLv) / Math.max(1, m.expNext - m.expLv), '#40a8f8');
    const evo = (sp.evo || [])[0];
    if (evo) {
      const how = evo.method === 0 ? 'Lv ' + evo.param : evo.method === 1 ? evo.item : (es ? 'intercambio' : 'trade');
      text(fitText((es ? 'Evoluciona: ' : 'Evolves: ') + evo.name + ' (' + how + ')', W - 22), 11, 116 + 6 * 18 + 10, INK_DIM);
    }
  }
  pageStats(es) {
    const m = this.m, base = (this.sp && this.sp.base) || [0, 0, 0, 0, 0];
    const names = es ? ['PS', 'ATAQUE', 'DEFENSA', 'VELOCIDAD', 'ESPECIAL'] : ['HP', 'ATTACK', 'DEFENSE', 'SPEED', 'SPECIAL'];
    const vals = [m.max, m.atk, m.def, m.spe, m.spc];
    const top = Math.max(...vals, 1);
    roundBox(3, 108, W - 6, 170, BOX.light);
    vals.forEach((v, i) => {
      const y = 116 + i * 30;
      textS(names[i], 11, y);
      textS(String(v), 229, y, INK, 'right');
      bar(11, y + 13, W - 100, 4, v / top, '#58a8e8');
      text((es ? 'base ' : 'base ') + base[i], 229, y + 11, INK_DIM, 'right');
    });
  }
  pageMoves() {
    const m = this.m;
    m.moves.forEach((mv, i) => {
      const y = 108 + i * 43;
      roundBox(3, y, W - 6, 41, BOX.light);
      textS(mv.name, 11, y + 6);
      typeChip(mv.type, 11, y + 17);
      textS(`PP ${mv.pp}/${mv.max}`, W - 11, y + 6, INK, 'right');
      text(fitText(moveDesc(mv), W - 22), 11, y + 30, INK_DIM);
    });
  }
  input(ev) {
    if (ev.k === 'back' || inBack(ev)) { pop(); return; }
    if (ev.k === 'up' || ev.k === 'left') this.page = (this.page + 2) % 3;
    else if (ev.k === 'down' || ev.k === 'right' || ev.k === 'ok') this.page = (this.page + 1) % 3;
    else if (ev.k === 'tap') this.page = ev.x < W / 2 ? (this.page + 2) % 3 : (this.page + 1) % 3;
  }
}

// ---------------------------------------------------- ficha de Pokedex ------
// La rueda pasa a la especie vista anterior o siguiente.

class DexEntryScreen {
  constructor(sp, seenList) {
    this.list = seenList;
    this.i = Math.max(0, seenList.indexOf(sp));
    this.load();
  }
  load() { this.sp = Engine.json('pt_species', this.list[this.i]); spriteImage(this.sp.sp); }
  draw(t) {
    const s = this.sp, es = lang === LANG_ES;
    menuBg();
    header('No.' + String(s.sp).padStart(3, '0') + ' ' + s.name, s.caught ? '●' : '○', true);
    // retrato
    roundBox(3, 22, W - 6, 96, BOX.light);
    ctx.save(); ctx.beginPath(); ctx.rect(6, 25, W - 12, 90); ctx.clip();
    ctx.translate(6, 25); ctx.scale((W - 12) / W, 90 / 176); drawBattleBg(s.tier === 3 ? 'forest' : 'grass', t, S.night, 176); ctx.restore();
    platform(W / 2, 108, 40, 8, 'grass', S.night);
    drawMon(s.sp, 'front', W / 2, 108, t, { scale: monScale(s.sp, s.hd, 'front', 44) });
    if (this.list.length > 1) { textS('◀', 12, 64, INK); textS('▶', W - 12, 64, INK, 'right'); }
    roundBox(3, 120, W - 6, 158, BOX.light);
    let x = typeChip(s.t1, 11, 127);
    if (s.t2 !== 255) typeChip(s.t2, 15 + x, 127);
    textS((s.hd / 10).toFixed(1) + ' m  ' + (s.wt / 10).toFixed(1) + ' kg', 229, 128, INK, 'right');
    // donde aparece
    const where = s.routes.length ? s.routes.join(', ')
      : [es ? 'No aparece en estado salvaje' : 'Not found in the wild', es ? 'Afuera, común' : 'Outside, common',
         es ? 'Afuera, poco común' : 'Outside, uncommon', es ? 'Afuera, zona lejana (raro)' : 'Outside, far zone (rare)'][s.tier] || '';
    textS(es ? 'Dónde:' : 'Where:', 11, 146, INK_DIM);
    wrap(where, 26).slice(0, 2).forEach((l, i) => text(l, 11, 158 + i * 10, INK));
    // evoluciones
    let y = 184;
    if (s.fromName) { text(fitText('← ' + s.fromName, W - 22), 11, y, INK_DIM); y += 12; }
    for (const e of s.evo.slice(0, 2)) {
      const how = e.method === 0 ? 'Lv ' + e.param : e.method === 1 ? e.item : (es ? 'intercambio' : 'trade');
      text(fitText('→ ' + e.name + ' (' + how + ')', W - 22), 11, y, INK_DIM);
      y += 12;
    }
    // stats base (solo si lo atrapaste, como la Pokedex completa del juego)
    if (s.caught) {
      const names = es ? ['PS', 'ATQ', 'DEF', 'VEL', 'ESP'] : ['HP', 'ATK', 'DEF', 'SPE', 'SPC'];
      s.base.forEach((v, i) => {
        const yy = 222 + i * 11;
        text(names[i], 11, yy, INK_DIM);
        bar(46, yy + 2, 150, 4, v / 160, '#58a8e8');
        text(String(v), 229, yy, INK, 'right');
      });
    } else text(es ? 'Atrápalo para ver sus stats.' : 'Catch it to see its stats.', 11, 232, INK_DIM);
  }
  input(ev) {
    if (ev.k === 'back' || inBack(ev)) { pop(); return; }
    const n = this.list.length;
    if (ev.k === 'up' || ev.k === 'left' || (ev.k === 'tap' && ev.x < 40)) { this.i = (this.i + n - 1) % n; this.load(); }
    else if (ev.k === 'down' || ev.k === 'right' || ev.k === 'ok' || (ev.k === 'tap' && ev.x > W - 40)) { this.i = (this.i + 1) % n; this.load(); }
  }
}

// --------------------------------------------- encuentro que salta solo ----
// En modo Afuera, si hay eventos y estas en el inicio, sale el "!" sobre tu
// companero y el encuentro aparece solo (como en GO), sin entrar a Explorar.

class AlertScreen {
  constructor() {
    this.t0 = performance.now();
    // con un temporizador (no en draw): asi avanza aunque el dibujo vaya lento
    setTimeout(() => {
      if (topScreen() !== this || this.done) return;
      this.done = true;
      // se quita sin "resume" del inicio: si no, el inicio pediria otro aviso
      stack.splice(stack.indexOf(this), 1);
      nextOutsideEvent();
    }, 900);
  }
  draw(t) {
    const under = stack[stack.indexOf(this) - 1];  // la de debajo de esta (no de la de arriba)
    if (under) under.draw(t);
    const k = Math.min(1, (t - this.t0) / 150);
    roundBox(14, 214 - 22 * k, 24, 22, BOX.light);
    textS('!', 26, 221 - 22 * k, '#c03028', 'center');
  }
  input() {}
}

function maybeAutoEvent() {
  if (prefs.autoEvents === false || !S || !S.outside || !(S.events + S.drops)) return;
  if (!(topScreen() instanceof HomeScreen) || stack.some(x => x instanceof AlertScreen)) return;
  push(new AlertScreen());
}
