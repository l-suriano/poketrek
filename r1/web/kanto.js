'use strict';
// Inicio con el mapa de Kanto (como el mapa del pueblo del juego): la rueda
// mueve el cursor de ciudad en ciudad, cada ciudad tiene sus edificios (Centro,
// Poke Mart, Gimnasio, Copa, rutas cercanas) y abajo va el objetivo siguiente.
// Todas las ciudades se pueden visitar (las copas estan abiertas desde el
// inicio, reglas: Tournaments); el avance lo marcan gimnasios y rutas.

const MAP_Y = 20;  // el mapa ocupa de y = 20 a 216

// x, y en el mapa; gym = indice en GYMS; cup = Cup; routes = indices de ROUTES
const CITIES = [
  { en: 'Pallet Town', icon: 'lab', es: 'Pueblo Paleta', x: 48, y: 128, gym: -1, cup: -1, routes: [0], lab: true },
  { en: 'Viridian City', icon: 'gym', es: 'Ciudad Verde', x: 48, y: 88, gym: 7, cup: -1, routes: [1] },
  { en: 'Pewter City', icon: 'museum', es: 'Ciudad Plateada', x: 48, y: 42, gym: 0, cup: 0, routes: [2] },
  { en: 'Cerulean City', icon: 'bluegym', es: 'Ciudad Celeste', x: 152, y: 32, gym: 1, cup: 1, routes: [6, 8] },
  { en: 'Saffron City', icon: 'silph', es: 'Ciudad Azafrán', x: 152, y: 78, gym: 5, cup: 4, routes: [], battleShop: true },
  { en: 'Celadon City', icon: 'dept', es: 'Ciudad Azulona', x: 102, y: 78, gym: 3, cup: 3, routes: [], store: true },
  { en: 'Vermilion City', icon: 'ship', es: 'Ciudad Carmín', x: 152, y: 122, gym: 2, cup: 2, routes: [] },
  { en: 'Lavender Town', icon: 'tower', es: 'Pueblo Lavanda', x: 206, y: 78, gym: -1, cup: -1, routes: [3] },
  { en: 'Fuchsia City', icon: 'safari', es: 'Ciudad Fucsia', x: 122, y: 162, gym: 4, cup: 5, routes: [4] },
  { en: 'Cinnabar Island', icon: 'volcano', es: 'Isla Canela', x: 48, y: 180, gym: 6, cup: 6, routes: [5] },
  { en: 'Indigo Plateau', icon: 'league', es: 'Meseta Añil', x: 16, y: 26, gym: -1, cup: 7, routes: [7], league: true },
];
// caminos entre ciudades (indices en CITIES); sea = ruta por mar
const PATHS = [[0, 1], [1, 2], [2, 3], [3, 4], [4, 6], [3, 7], [7, 4], [4, 5], [5, 8], [7, 8], [1, 10]];
const SEA_PATHS = [[8, 9], [9, 0]];

function cityName(c) { return lang === LANG_ES ? c.es : c.en; }

// --------------------------------------------------------------- objetivo ---
// La linea de "que sigue": siempre dice al jugador que hacer ahora.

function nextGoal(gyms) {
  if (!S.party.some(m => m.hp > 0)) return { text: L('goalHeal'), city: 0 };
  if (S.events + S.drops) return { text: L('goalEvents', S.events + S.drops), city: -1 };
  if (gyms) {
    const i = gyms.gyms.findIndex(g => !g.badge);
    if (i >= 0) {
      const g = gyms.gyms[i];
      const city = CITIES.findIndex(c => c.gym === i);
      if (g.open) return { text: L('goalGym', g.leader, cityName(CITIES[city])), city };
      if (i > 0 && !gyms.gyms[i - 1].badge) return { text: L('goalGym', gyms.gyms[i - 1].leader, ''), city };
      return { text: L('goalKm', (g.km - gyms.km), g.leader), city };
    }
    if (!S.champion) {
      if (!gyms.leagueOpen) return { text: L('goalLeagueKm', Math.max(0, 80 - gyms.km)), city: 10 };
      return { text: L('goalLeague'), city: 10 };
    }
  }
  return { text: L('goalDex', S.caught), city: -1 };
}

// ------------------------------------------------------------------ inicio --

class HomeScreen {
  constructor() {
    this.sel = 0;  // ciudad seleccionada; CITIES.length = boton MENU
    this.info = {};
    this.goal = { text: '', city: -1 };
  }
  enter() { this.build(); }
  resume() { this.build(); }
  build() {
    refresh();
    const g = Engine.json('pt_gyms');
    this.info = { gyms: g.gyms, km: g.km, leagueOpen: g.leagueOpen };
    this.goal = nextGoal(g);
    this.info.goalCity = this.goal.city;
    checkPending(() => checkRival());
  }
  // camara sobre el mundo: sigue a la ciudad elegida (o se queda donde estaba con MENU)
  camera(t) {
    const c = CITIES[Math.min(this.sel, CITIES.length - 1)];
    const tx = Math.max(0, Math.min(WORLD_W - W, wx(c) - W / 2));
    const ty = Math.max(0, Math.min(WORLD_H - 196, wy(c) - 100));
    if (!this.cam) this.cam = { x: tx, y: ty };
    if (this.sel < CITIES.length) {
      this.cam.x += (tx - this.cam.x) * 0.18;
      this.cam.y += (ty - this.cam.y) * 0.18;
    }
    return this.cam;
  }
  draw(t) {
    rect(0, 0, W, H, '#101420');
    if (!worldCanvas) buildWorld();
    const cam = this.camera(t);
    const cx = Math.round(cam.x), cy = Math.round(cam.y);
    ctx.drawImage(worldCanvas, cx, cy, W, 196, 0, MAP_Y, W, 196);
    // agua que brilla, sobre lo ya dibujado
    if (S.night) { ctx.fillStyle = 'rgba(16,24,72,0.4)'; ctx.fillRect(0, MAP_Y, W, 196); }
    // estado de cada gimnasio sobre su pueblo y el objetivo que parpadea
    CITIES.forEach((c, i) => {
      const sx = wx(c) - cx, sy = wy(c) - cy + MAP_Y;
      if (sx < -50 || sx > W + 50 || sy < MAP_Y - 50 || sy > MAP_Y + 246) return;
      let tag = null;
      if (c.gym >= 0 && this.info.gyms) {
        const g = this.info.gyms[c.gym];
        tag = g.badge ? ['★', '#f8d030', '#806010'] : g.open ? ['!', '#f85838', '#601810'] : null;
      } else if (c.league && this.info.leagueOpen) tag = ['!', '#a060e0', '#301848'];
      if (i === this.info.goalCity && Math.floor(t / 400) % 2) tag = ['!', '#ffffff', '#c03028'];
      if (tag) {
        roundBox(sx + 30, sy - 50, 16, 16, BOX.light);
        textS(tag[0], sx + 38, sy - 46, tag[1], 'center', tag[2]);
      }
    });
    header(profile.player, money(S.money));
    // cursor y cartel con el nombre
    if (this.sel < CITIES.length) {
      const c = CITIES[this.sel];
      const sx = wx(c) - cx, sy = wy(c) - cy + MAP_Y;
      const k = Math.floor(t / 250) % 2;
      ctx.strokeStyle = '#c03028'; ctx.lineWidth = 2;
      ctx.strokeRect(sx - 49 - k, sy - 39 - k, 98 + k * 2, 80 + k * 2);
      setFont(8);
      const label = cityName(c);
      const w = ctx.measureText(label).width + 14;
      const bx = Math.max(2, Math.min(W - w - 2, sx - w / 2));
      const by = Math.max(MAP_Y + 2, Math.min(MAP_Y + 176, sy + 44));
      roundBox(bx, by, w, 16, BOX.info);
      textS(label, bx + 7, by + 4);
    }
    // panel de abajo: companero, objetivo y boton MENU
    roundBox(0, 216, W, 66, BOX.light);
    const p = partner();
    if (p) drawMon(p.sp, 'front', 26, 262, t, { scale: Math.min(1.2, monScale(p.sp, p.hd, 'front', 26)) });
    wrap(this.goal.text, 20).slice(0, 3).forEach((l, i) => text(l, 50, 224 + i * 11, INK));
    const menuOn = this.sel === CITIES.length;
    roundBox(W - 70, 258, 66, 20, menuOn ? BOX.sel : BOX.info);
    if (menuOn) cursor(W - 64, 264, INK);
    textS(L('menu') + (S.events + S.drops ? ' !' : ''), W - 34, 264, S.events + S.drops ? '#c03028' : INK, 'center');
    textS('EE ' + S.ee, 50, 266, INK_DIM);
  }
  input(ev) {
    const n = CITIES.length + 1;
    if (ev.k === 'up' || ev.k === 'left') this.sel = (this.sel + n - 1) % n;
    else if (ev.k === 'down' || ev.k === 'right') this.sel = (this.sel + 1) % n;
    else if (ev.k === 'ok') this.open(this.sel);
    else if (ev.k === 'tap') {
      if (ev.x > W - 74 && ev.y > 254) { this.sel = CITIES.length; this.open(this.sel); return; }
      if (ev.y >= MAP_Y && ev.y < 216 && this.cam) {
        // tocar un pueblo: lo elige; tocarlo otra vez, entra
        const mx = ev.x + this.cam.x, my = ev.y - MAP_Y + this.cam.y;
        const best = CITIES.findIndex(c => Math.abs(mx - wx(c)) < 50 && Math.abs(my - wy(c)) < 42);
        if (best >= 0) { if (best === this.sel) this.open(best); else this.sel = best; }
      } else if (ev.y > 216 && this.goal.city >= 0) this.sel = this.goal.city;  // tocar el objetivo lleva a su ciudad
    }
  }
  open(i) {
    if (i === CITIES.length) push(mainMenu());
    else push(cityScreen(i));
  }
}

// ------------------------------------------------------------- menu (MENU) --

function mainMenu() {
  const build = () => {
    refresh();
    return [
      { label: L('explore'), right: S.events + S.drops ? '!' + (S.events + S.drops) : '', rightColor: '#c03028', action: () => push(exploreScreen()) },
      { label: L('pokemon'), right: S.party.length + '/6', action: () => push(partyScreen('menu')) },
      { label: L('bag'), action: () => push(bagScreen()) },
      { label: L('pokedex'), right: S.caught + '/151', action: () => push(dexScreen()) },
      { label: L('tournaments'), right: S.dcPlayed ? '' : '!', rightColor: '#c03028', action: () => push(tourScreen()) },
      { label: L('pc'), right: S.boxCount, action: () => push(boxScreen()) },
      { label: L('settings'), action: () => push(settingsScreen()) },
    ];
  };
  return new ListScreen(L('menu'), build(), { refresh: build, right: () => money(S.money) });
}

// ---------------------------------------------------------------- ciudad ----

function cityScreen(ci) {
  const c = CITIES[ci];
  const build = () => {
    refresh();
    const g = Engine.json('pt_gyms');
    const items = [{ label: L('center'), action: () => pokemonCenter(() => topScreen().resume && topScreen().resume()) }];
    if (c.lab) items.push({ label: L('oakLab'), sub: L('pokedex') + ' ' + S.caught + '/151', action: () => push(dexScreen()) });
    if (!c.lab) items.push({ label: L('mart'), action: () => push(new ListScreen(L('mart'), [
      { label: L('buy'), action: () => push(shopScreen(0, L('mart'))) },
      { label: L('sell'), action: () => push(shopScreen(2, L('sell'))) },
    ], { right: () => money(S.money) })) });
    if (c.store) items.push({ label: L('celadon'), action: () => push(shopScreen(1, L('celadon'))) });
    if (c.battleShop) items.push({ label: L('battleShop'), right: S.bp + ' BP', action: () => push(shopScreen(3, L('battleShop'))) });
    if (c.gym >= 0) {
      const gym = g.gyms[c.gym];
      items.push({
        label: L('gymOf', gym.leader), right: gym.badge ? '★' : gym.open ? typeName(gym.type) : L('locked'),
        rightColor: gym.badge ? '#c09020' : INK_DIM,
        sub: gym.open ? '' : (c.gym > 0 && !g.gyms[c.gym - 1].badge ? L('needBadge') : L('kmToOpen', gym.km - g.km)),
        disabled: !gym.open, why: L('gymClosed'),
        action: () => ask(gym.leader + ' · ' + typeName(gym.type), [L('challenge'), L('back')], k => {
          if (k === 0) startBattle(2, c.gym, () => topScreen().resume && topScreen().resume());
        }),
      });
    }
    if (c.league) {
      items.push({
        label: L('league'), right: g.leagueActive ? (g.leagueIdx + 1) + '/5' : '',
        sub: g.leagueOpen ? (g.leagueActive ? L('leagueNext', g.elite[g.leagueIdx].name) : '') : L('leagueNeed'),
        disabled: !g.leagueOpen, why: L('leagueNeed'),
        action: () => {
          const opts = g.leagueActive ? [L('leagueNext', g.elite[g.leagueIdx].name), L('pokemon'), L('bag'), L('leagueQuit'), L('back')] : [L('challenge'), L('back')];
          ask(L('league'), opts, k => {
            if (k === 0) startBattle(3, 0, () => topScreen().resume && topScreen().resume());
            else if (g.leagueActive && k === 1) push(partyScreen('menu'));
            else if (g.leagueActive && k === 2) push(bagScreen());
            else if (g.leagueActive && k === 3) { Engine.call('pt_league_quit'); saveSoon(); topScreen().resume(); }
          });
        },
      });
    }
    if (c.cup >= 0 && (c.cup !== 7 || S.champion)) {
      const d = Engine.json('pt_tour_info');
      const feat = d.featured === c.cup;
      const busy = d.active && !d.over;
      items.push({
        label: d.cups[c.cup].name, right: feat ? '★' : '', rightColor: '#c09020',
        sub: feat ? L('featured') + (d.featuredType !== 255 ? ' ' + typeName(d.featuredType) : '') : '',
        disabled: busy, why: L('continueTour'),
        action: () => push(tierScreen(c.cup, d.cups[c.cup].name, d, feat)),
      });
    }
    const routes = c.routes.length ? Engine.json('pt_routes') : [];
    for (const r of c.routes) {
      const ro = routes[r];
      items.push({
        label: ro.name, right: ro.open ? '300 EE' : (ro.badges === 9 ? L('afterChampion') : ro.badges + ' ★'),
        disabled: !ro.open, why: L('routeLocked'),
        action: () => {
          const e = Engine.json('pt_search', r);
          encounterBg = ROUTE_BG[r] || 'grass';
          saveSoon();
          handleEvent(e);
        },
      });
    }
    return items;
  };
  return new ListScreen(cityName(c), build(), { rowH: 24, refresh: build, right: () => 'EE ' + S.ee });
}
