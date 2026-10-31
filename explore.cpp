#include "explore.h"
#include "gyms.h"
#include <math.h>
#include <string.h>

// ------------------------------------------------------------------ tablas --

// rareza como salvaje y como huevo: 1 comun, 2 media, 3 rara (solo zona lejana
// y huevos Muy Raros). Formas base; los iniciales, los fosiles y los Pokemon de
// regalo de gen 1 son raros. Los legendarios salen solo en sus rutas.
const uint8_t WILD_TIER[152] = {
  0, 3, 0, 0, 3, 0, 0, 3, 0, 0,  // 0-9
  1, 0, 0, 1, 0, 0, 1, 0, 0, 1,  // 10-19
  0, 1, 0, 1, 0, 2, 0, 1, 0, 1,  // 20-29
  0, 0, 1, 0, 0, 2, 0, 2, 0, 1,  // 30-39
  0, 1, 0, 1, 0, 0, 1, 0, 1, 0,  // 40-49
  1, 0, 1, 0, 1, 0, 1, 0, 2, 0,  // 50-59
  1, 0, 0, 2, 0, 0, 1, 0, 0, 1,  // 60-69
  0, 0, 1, 0, 1, 0, 0, 2, 0, 2,  // 70-79
  0, 1, 0, 2, 1, 0, 2, 0, 2, 0,  // 80-89
  2, 0, 2, 0, 0, 2, 1, 0, 1, 0,  // 90-99
  1, 0, 2, 0, 2, 0, 3, 3, 3, 1,  // 100-109
  0, 2, 0, 3, 2, 2, 1, 0, 1, 0,  // 110-119
  2, 0, 3, 2, 3, 3, 3, 2, 2, 1,  // 120-129
  0, 3, 2, 3, 0, 0, 0, 3, 3, 0,  // 130-139
  3, 0, 3, 3, 0, 0, 0, 3, 0, 0,  // 140-149
  0, 0,  // 150-151
};

// Rutas desde casa. Cada una junta las zonas de alrededor del juego original
// (Rock Tunnel tambien trae las rutas 5-10 y la Torre Pokemon) para que quien
// juega sentado vea variedad. Solo comunes y medias: las raras son de afuera.
const RouteInfo ROUTES[NUM_ROUTES] = {
  { "Route 1", "Ruta 1", 0, 0, 0,
    { {16, 30}, {19, 30}, {21, 12}, {29, 10}, {32, 10}, {56, 8} } },
  { "Viridian Forest", "Bosque Verde", 0, 0, 0,
    { {10, 28}, {13, 28}, {16, 14}, {21, 10}, {25, 8}, {39, 7} } },
  { "Mt. Moon", "Mt. Moon", 1, 0, 0,
    { {41, 32}, {74, 24}, {46, 14}, {27, 9}, {23, 8}, {35, 8}, {50, 5} } },
  { "Rock Tunnel", "T\243nel Roca", 3, 0, 0,
    { {41, 18}, {74, 16}, {66, 14}, {96, 10}, {43, 8}, {69, 8}, {95, 7}, {52, 7},
      {63, 6}, {92, 5}, {58, 4}, {37, 4} } },
  { "Safari Zone", "Zona Safari", 5, 0, 0,
    { {102, 12}, {111, 10}, {48, 10}, {84, 10}, {29, 9}, {32, 9}, {77, 6}, {115, 4},
      {123, 4}, {127, 4}, {128, 4}, {114, 4} } },
  { "Seafoam Islands", "Islas Espuma", 8, 144, 5,
    { {86, 22}, {54, 12}, {79, 12}, {116, 9}, {98, 9}, {41, 8}, {90, 8}, {72, 8},
      {120, 6}, {60, 6}, {118, 6}, {129, 6} } },
  { "Power Plant", "Central El\202ctrica", 8, 145, 5,
    { {81, 28}, {100, 26}, {25, 18}, {88, 14}, {109, 14} } },
  { "Victory Road", "Calle Victoria", 8, 146, 5,
    { {66, 18}, {74, 18}, {95, 14}, {41, 14}, {48, 12}, {104, 12} } },
  { "Cerulean Cave", "Cueva Celeste", ROUTE_CHAMPION, 150, 10,
    { {132, 16}, {63, 10}, {100, 10}, {111, 10}, {41, 10}, {66, 10}, {46, 6}, {39, 6},
      {35, 6}, {48, 6}, {81, 6} } },
};

static const uint8_t ROUTE_LEGENDS[NUM_ROUTE_LEGENDS] = { 144, 145, 146, 150 };

// dinero base de gen 1 y equipos por tipo (formas base)
const TrainerClass TRAINER_CLASSES[NUM_TRAINER_CLASSES] = {
  { "Youngster", "Joven", 15, 0, {19, 21, 23, 27, 32, 16} },
  { "Bug Catcher", "Cazabichos", 10, 0, {10, 13, 46, 48} },
  { "Lass", "Chica", 15, 0, {16, 39, 43, 35, 29, 52} },
  { "Hiker", "Monta\244ero", 35, 0, {74, 66, 95, 104} },
  { "Fisherman", "Pescador", 35, 0, {129, 60, 118, 72, 90, 98} },
  { "Swimmer", "Nadador", 5, 0, {72, 120, 116, 86, 54} },
  { "Bird Keeper", "Ornit\242logo", 25, 0, {16, 21, 84, 83} },
  { "Super Nerd", "Supernecio", 25, 10, {81, 100, 88, 109} },
  { "Team Rocket", "Recluta Rocket", 30, 10, {19, 41, 23, 109, 88, 96} },
  { "Engineer", "Operario", 50, 15, {81, 100} },
  { "Psychic", "Ps\241quico", 10, 15, {63, 96, 79, 102} },
  { "Black Belt", "Karateka", 25, 15, {66, 56} },
  { "Channeler", "M\202dium", 30, 20, {92} },
  { "Gentleman", "Caballero", 70, 25, {58, 25, 77, 81} },
  { "Beauty", "Bella", 70, 25, {43, 69, 102, 35, 114} },
  { "Cooltrainer", "Entrenador Guay", 35, 30, {58, 37, 77, 25, 111, 102} },
};

const char *routeName(uint8_t route) {
  if (route >= NUM_ROUTES) return "";
  return gLang == LANG_ES ? ROUTES[route].es : ROUTES[route].en;
}

bool routeOpen(const Trainer &t, uint8_t route) {
  if (route >= NUM_ROUTES) return false;
  if (ROUTES[route].badges == ROUTE_CHAMPION) return t.champion;
  return badgeCount(t) >= ROUTES[route].badges;
}

uint16_t eggKm(uint8_t tier) { return tier == EGG_VERY_RARE ? 10 : tier == EGG_RARE ? 5 : 2; }

uint8_t rewardTier(uint8_t level) { return level < 20 ? 0 : level < 35 ? 1 : level < 50 ? 2 : 3; }

// ----------------------------------------------------------------- basicos --

void exploreInit(Explore &ex) { ex = Explore(); }

uint8_t partyBaseLevel(const Trainer &t) {
  BattleMon tmp[PARTY_MAX];
  for (int i = 0; i < t.partyCount; i++) tmp[i].level = t.party[i].level;
  return baseLevel(tmp, t.partyCount);
}

uint16_t dayIndex(uint32_t now) { return (uint16_t)(now / 86400UL); }

static uint8_t clampLevel(int v) { return (uint8_t)(v < 2 ? 2 : v > 100 ? 100 : v); }

static bool rollOneIn(Rng &rng, uint16_t den) { return den <= 1 || rng.next() % den == 0; }

// ------------------------------------------------------------------ caminar -

bool freeRareCandy(Explore &ex, Trainer &t, uint32_t now) {
  uint16_t day = dayIndex(now);
  if (ex.candyDay == day) return false;
  ex.candyDay = day;
  addItem(t, IT_RARE_CANDY);
  return true;
}

WalkResult exploreWalk(Explore &ex, Trainer &t, const WalkInput &in) {
  WalkResult r;
  if (in.speedKmh > MAX_SPEED_KMH) return r;  // coche o bus: no cuenta nada

  ex.steps += in.steps;
  uint16_t room = EE_MAX - ex.ee;
  r.ee = in.steps < room ? in.steps : room;
  ex.ee += r.ee;

  if (in.outside) {
    for (uint16_t n = in.steps; n; ) {
      uint16_t d = n < ex.stepsToEvent ? n : ex.stepsToEvent;
      ex.stepsToEvent -= d; n -= d;
      if (!ex.stepsToEvent) { r.events++; ex.stepsToEvent = STEPS_PER_EVENT; }
    }
    if (!in.gps) {
      for (uint16_t n = in.steps; n; ) {
        uint16_t d = n < ex.stepsToDrop ? n : ex.stepsToDrop;
        ex.stepsToDrop -= d; n -= d;
        if (!ex.stepsToDrop) { r.drops++; ex.stepsToDrop = STEPS_PER_DROP; }
      }
    }
  }

  uint32_t meters = in.meters;
  if (!in.gps) {
    uint32_t mm = (uint32_t)in.steps * STEP_LEN_MM + ex.mmCarry;
    meters = mm / 1000;
    ex.mmCarry = (uint16_t)(mm % 1000);
  }
  ex.meters += meters;

  for (int i = 0; i < MAX_EGGS; i++) {
    Egg &e = ex.eggs[i];
    if (!e.species) continue;
    uint32_t goal = (uint32_t)eggKm(e.tier) * 1000;
    uint32_t w = e.walkedM + meters;
    e.walkedM = (uint16_t)(w > goal ? goal : w);
    if (e.walkedM >= goal) r.eggsReady++;
  }

  uint32_t km = ex.meters / 1000;
  if (km > ex.kmPaid) {
    r.km = (uint16_t)(km - ex.kmPaid);
    for (int i = 0; i < t.partyCount; i++) trustPerKm(t.party[i], r.km);
    // un Caramelo Raro cada 10 km, dentro del tope diario
    if (km / 10 > ex.kmPaid / 10 && freeRareCandy(ex, t, in.now)) r.candies = 1;
    ex.kmPaid = km;
  }
  return r;
}

// ----------------------------------------------------------------- geografia

uint32_t geoDistanceM(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2) {
  // equirrectangular: de sobra para distancias de pocos km
  const float R = 6371000.0f, K = 3.14159265f / 180e6f;
  float la1 = lat1 * K, la2 = lat2 * K;
  float dx = (float)(lon2 - lon1) * K * cosf((la1 + la2) * 0.5f);
  float dy = la2 - la1;
  return (uint32_t)(sqrtf(dx * dx + dy * dy) * R + 0.5f);
}

uint8_t exploreZone(Explore &ex, bool gps, int32_t latE6, int32_t lonE6) {
  if (gps) {
    if (!ex.homeSet) return ZONE_NEAR;
    return geoDistanceM(ex.homeLatE6, ex.homeLonE6, latE6, lonE6) >= NEAR_ZONE_M ? ZONE_FAR : ZONE_NEAR;
  }
  // sin GPS: uno de cada 5 eventos usa la lista lejana
  return (++ex.outdoorEvents % 5 == 0) ? ZONE_FAR : ZONE_NEAR;
}

// --------------------------------------------------------------- salvajes ---

uint8_t wildForm(uint8_t species, uint8_t level) {
  for (bool again = true; again; ) {
    again = false;
    for (int i = 0; i < NUM_EVOS; i++) {
      const EvoEntry &e = EVO_TBL[i];
      if (e.from == species && e.method == EVO_LEVEL && level >= e.param) {
        species = e.to;
        again = true;
        break;
      }
    }
  }
  return species;
}

void exploreWild(BattleMon &m, uint8_t species, uint8_t level, Rng &rng) {
  uint8_t genes[5];
  for (int i = 0; i < 5; i++) genes[i] = (uint8_t)rng.range(90, 110);
  monInit(m, wildForm(species, level), level, genes);
}

static uint8_t pickTierSpecies(Rng &rng, uint8_t tier) {
  uint8_t cand[40];
  uint8_t n = 0;
  for (int s = 1; s <= 151 && n < sizeof(cand); s++)
    if (WILD_TIER[s] == tier) cand[n++] = (uint8_t)s;
  return n ? cand[rng.range(0, n - 1)] : 16;
}

static uint8_t pickRouteSpecies(Rng &rng, uint8_t route) {
  const RouteInfo &r = ROUTES[route];
  int total = 0;
  for (int i = 0; i < ROUTE_POOL && r.pool[i].weight; i++) total += r.pool[i].weight;
  int x = rng.range(0, total - 1);
  for (int i = 0; i < ROUTE_POOL && r.pool[i].weight; i++) {
    if (x < r.pool[i].weight) return r.pool[i].species;
    x -= r.pool[i].weight;
  }
  return r.pool[0].species;
}

static void makeWild(Encounter &e, Trainer &t, Rng &rng, uint8_t species, int lo, int hi,
                     uint16_t shinyDen) {
  e.kind = EN_WILD;
  exploreWild(e.wild, species, clampLevel(rng.range(lo, hi)), rng);
  e.shiny = rollOneIn(rng, shinyDen);
  dexMark(t.seen, e.wild.species);
}

// ---------------------------------------------------------- entrenadores ----

void buildRouteTrainer(RouteTrainer &rt, uint8_t bl, Rng &rng) {
  rt = RouteTrainer();
  uint8_t cand[NUM_TRAINER_CLASSES];
  uint8_t n = 0;
  for (int i = 0; i < NUM_TRAINER_CLASSES; i++)
    if (bl >= TRAINER_CLASSES[i].minBl) cand[n++] = (uint8_t)i;
  rt.cls = cand[rng.range(0, n - 1)];
  const TrainerClass &c = TRAINER_CLASSES[rt.cls];
  uint8_t poolN = 0;
  while (poolN < 6 && c.pool[poolN]) poolN++;

  rt.count = (uint8_t)rng.range(1, bl < 15 ? 2 : 3);
  uint8_t lv[ROUTE_TRAINER_MAX];
  for (int i = 0; i < rt.count; i++) lv[i] = clampLevel(rng.range(bl - 3, bl + 2));
  // de menor a mayor: el ultimo es el mas fuerte y fija el premio
  for (int i = 1; i < rt.count; i++)
    for (int j = i; j > 0 && lv[j] < lv[j - 1]; j--) { uint8_t x = lv[j]; lv[j] = lv[j - 1]; lv[j - 1] = x; }
  for (int i = 0; i < rt.count; i++) {
    uint8_t base = c.pool[rng.range(0, poolN - 1)];
    uint8_t genes[5];
    for (int k = 0; k < 5; k++) genes[k] = (uint8_t)rng.range(90, 110);
    monInit(rt.party[i], evolveForLevel(base, lv[i]), lv[i], genes);
  }
  rt.aiTier = aiTierForLevel(lv[rt.count - 1]);
  rt.prize = prizeMoney(c.prizeBase, lv[rt.count - 1]);
}

void routeTrainerToSide(const RouteTrainer &rt, BattleSide &side) {
  side = BattleSide();
  for (int i = 0; i < rt.count; i++) side.party[i] = rt.party[i];
  side.count = rt.count;
  side.aiTier = rt.aiTier;
  side.potions = 0;  // los entrenadores de ruta no curan
}

static void makeTrainer(Encounter &e, Trainer &t, Rng &rng, uint8_t bl) {
  e.kind = EN_TRAINER;
  buildRouteTrainer(e.trainer, bl, rng);
  for (int i = 0; i < e.trainer.count; i++) dexMark(t.seen, e.trainer.party[i].species);
}

// ----------------------------------------------------------------- objetos --

static const uint8_t BALL_BY_TIER[4] = { IT_POKE_BALL, IT_GREAT_BALL, IT_ULTRA_BALL, IT_ULTRA_BALL };
static const uint8_t HEAL_BY_TIER[4][3] = {
  { IT_POTION, IT_ANTIDOTE, IT_POTION },
  { IT_SUPER_POTION, IT_AWAKENING, IT_PARALYZE_HEAL },
  { IT_HYPER_POTION, IT_REVIVE, IT_HYPER_POTION },
  { IT_FULL_RESTORE, IT_MAX_REVIVE, IT_FULL_RESTORE },
};
static const uint8_t STONES[5] = { IT_FIRE_STONE, IT_WATER_STONE, IT_THUNDER_STONE, IT_LEAF_STONE,
                                   IT_MOON_STONE };

static uint8_t healItem(Rng &rng, uint8_t tier) { return HEAL_BY_TIER[tier][rng.range(0, 2)]; }

void findItem(Explore &ex, Trainer &t, Rng &rng, uint8_t level, bool outside, uint32_t now,
              uint8_t &item, uint8_t &count) {
  uint8_t tier = rewardTier(level);
  count = 1;
  int r = rng.range(0, 99);
  if (outside && r < 2) {
    item = IT_NUGGET;
  } else if (r < 40) {
    item = BALL_BY_TIER[tier];
    count = (uint8_t)rng.range(1, 2);
  } else if (r < 85 || tier == 0) {
    item = healItem(rng, tier);
  } else if (tier == 1) {
    item = ITEM_BERRY;  // bayas raras para el Companero
    count = (uint8_t)rng.range(1, 2);
    return;
  } else if (tier == 2) {
    item = STONES[rng.range(0, 4)];
  } else {
    // Caramelo Raro dentro del tope diario; si no, Restaurar Todo
    if (freeRareCandy(ex, t, now)) { item = IT_RARE_CANDY; return; }
    item = IT_FULL_RESTORE;
  }
  addItem(t, item, count);
}

static void lootAdd(Loot &out, uint8_t item, uint8_t n) {
  for (int i = 0; i < out.count; i++)
    if (out.item[i] == item) { out.n[i] += n; return; }
  if (out.count < LOOT_MAX) { out.item[out.count] = item; out.n[out.count] = n; out.count++; }
}

static void merchantRoll(const Trainer &t, Rng &rng, Loot &out) {
  out.merchant = true;
  if (rng.chance(50)) {
    // tres objetos de la tienda con un 30 % de descuento
    uint8_t cand[IT_COUNT];
    uint8_t n = 0;
    for (int i = 0; i < IT_COUNT; i++)
      if (ITEMS[i].price && martSells(t, (uint8_t)i)) cand[n++] = (uint8_t)i;
    for (int k = 0; k < MERCHANT_ITEMS; k++) {
      uint8_t j = (uint8_t)rng.range(k, n - 1);  // sin repetir
      uint8_t x = cand[k]; cand[k] = cand[j]; cand[j] = x;
      out.offer[k].item = cand[k];
      out.offer[k].price = (uint16_t)(ITEMS[cand[k]].price * 7 / 10);
    }
  } else {
    // mercancia rara que la tienda no tiene
    out.offer[0] = { IT_MOON_STONE, 3000 };
    out.offer[1] = { IT_INCENSE, 1000 };
    out.offer[2] = { STONES[rng.range(0, 3)], 2100 };
  }
}

void exploreLoot(Explore &ex, Trainer &t, Rng &rng, Loot &out, uint16_t eggShinyDen) {
  out = Loot();
  uint8_t tier = rewardTier(partyBaseLevel(t));
  int items = rng.range(3, 5);
  for (int i = 0; i < items; i++) {
    int r = rng.range(0, 99);
    if (r < 3 && !out.eggTier) {
      int e = rng.range(0, 99);
      uint8_t et = e < 70 ? EGG_COMMON : e < 95 ? EGG_RARE : EGG_VERY_RARE;
      if (eggAdd(ex, rng, et, eggShinyDen)) { out.eggTier = et; continue; }
    }
    if (r < 8) lootAdd(out, IT_INCENSE, 1);
    else if (r < 53) lootAdd(out, BALL_BY_TIER[tier], (uint8_t)rng.range(1, 3));
    else if (r < 80) lootAdd(out, healItem(rng, tier), 1);
    else out.berries += (uint8_t)rng.range(1, 2);
  }
  for (int i = 0; i < out.count; i++) addItem(t, out.item[i], out.n[i]);
  if (rng.chance(10)) merchantRoll(t, rng, out);
}

bool merchantBuy(Trainer &t, const Loot &loot, uint8_t idx) {
  if (!loot.merchant || idx >= MERCHANT_ITEMS) return false;
  if (!spend(t, loot.offer[idx].price)) return false;
  addItem(t, loot.offer[idx].item);
  return true;
}

// ------------------------------------------------------------- encuentros ---

Encounter exploreOutdoorEvent(Explore &ex, Trainer &t, Rng &rng, uint8_t zone, uint32_t now,
                              uint16_t shinyDen) {
  Encounter e;
  e.zone = zone;
  uint8_t bl = partyBaseLevel(t);
  int r = rng.range(0, 99);
  if (r < 60) {
    // cercana: casi todo comunes; lejana: tambien raras y mas nivel
    int x = rng.range(0, 99);
    if (zone == ZONE_FAR) {
      uint8_t tier = x < 45 ? WT_COMMON : x < 80 ? WT_MID : WT_RARE;
      makeWild(e, t, rng, pickTierSpecies(rng, tier), bl - 2, bl + 8, shinyDen);
    } else {
      uint8_t tier = x < 85 ? WT_COMMON : WT_MID;
      makeWild(e, t, rng, pickTierSpecies(rng, tier), bl - 4, bl + 1, shinyDen);
    }
    e.expiresAt = now + WILD_LINGER_S;
  } else if (r < 75) {
    makeTrainer(e, t, rng, bl);
  } else if (r < 90) {
    e.kind = EN_ITEM;
    findItem(ex, t, rng, bl, true, now, e.item, e.itemCount);
  }
  return e;
}

static int8_t legendIndex(uint8_t species) {
  for (int i = 0; i < NUM_ROUTE_LEGENDS; i++) if (ROUTE_LEGENDS[i] == species) return (int8_t)i;
  return -1;
}

static bool legendAvailable(const Explore &ex, int8_t li, uint32_t now) {
  return li >= 0 && !(ex.legendCaught & (1 << li)) && now >= ex.legendBackAt[li];
}

bool exploreSearchRoute(Explore &ex, Trainer &t, Rng &rng, uint8_t route, uint32_t now,
                        Encounter &out, uint16_t shinyDen) {
  out = Encounter();
  if (!routeOpen(t, route) || ex.ee < EE_PER_SEARCH) return false;
  ex.ee -= EE_PER_SEARCH;
  const RouteInfo &ri = ROUTES[route];
  uint8_t bl = partyBaseLevel(t);

  int8_t li = legendIndex(ri.legend);
  if (legendAvailable(ex, li, now) && rng.chance(LEGEND_PCT)) {
    out.kind = EN_LEGEND;
    out.legend = (uint8_t)li;
    uint8_t genes[5] = { 110, 110, 110, 110, 110 };
    monInit(out.wild, ri.legend, clampLevel(bl + ri.legendPlus), genes);
    out.shiny = rollOneIn(rng, shinyDen);
    dexMark(t.seen, ri.legend);
    return true;
  }

  int r = rng.range(0, 99);
  if (r < 70) makeWild(out, t, rng, pickRouteSpecies(rng, route), bl - 5, bl, shinyDen);
  else if (r < 90) makeTrainer(out, t, rng, bl);
  else { out.kind = EN_ITEM; findItem(ex, t, rng, bl, false, now, out.item, out.itemCount); }
  return true;
}

void exploreLegendResult(Explore &ex, uint8_t legend, bool caught, uint32_t now) {
  if (legend >= NUM_ROUTE_LEGENDS) return;
  if (caught) ex.legendCaught |= (uint8_t)(1 << legend);
  else ex.legendBackAt[legend] = now + LEGEND_BACK_S;
}

// ---------------------------------------------------------------- incienso --

bool incenseStart(Explore &ex, Trainer &t, uint8_t route, uint32_t now) {
  if (!routeOpen(t, route) || !t.bag[IT_INCENSE] || incenseActive(ex, now)) return false;
  t.bag[IT_INCENSE]--;
  ex.incenseRoute = route;
  ex.incenseUntil = now + INCENSE_S;
  ex.incenseNext = now;  // el primero sale enseguida
  return true;
}

bool incenseActive(const Explore &ex, uint32_t now) { return now < ex.incenseUntil; }

bool incenseTick(Explore &ex, Trainer &t, Rng &rng, uint32_t now, Encounter &out, uint16_t shinyDen) {
  if (!incenseActive(ex, now) || now < ex.incenseNext) return false;
  // si la placa estuvo dormida, los que tocaban mientras tanto se pierden
  while (ex.incenseNext <= now) ex.incenseNext += INCENSE_EVERY_S;
  out = Encounter();
  uint8_t bl = partyBaseLevel(t);
  makeWild(out, t, rng, pickRouteSpecies(rng, ex.incenseRoute), bl - 5, bl, shinyDen);
  out.expiresAt = ex.incenseNext;
  return true;
}

// -------------------------------------------------------------- PokeStops ---

void exploreSetHome(Explore &ex, int32_t latE6, int32_t lonE6) {
  ex.homeSet = true;
  ex.homeLatE6 = latE6;
  ex.homeLonE6 = lonE6;
}

StopResult stopPlace(Explore &ex, int32_t latE6, int32_t lonE6) {
  if (!ex.homeSet) return STOP_NO_HOME;
  if (ex.stopCount >= MAX_STOPS) return STOP_FULL;
  if (geoDistanceM(ex.homeLatE6, ex.homeLonE6, latE6, lonE6) < STOP_MIN_GAP_M) return STOP_TOO_CLOSE;
  for (int i = 0; i < ex.stopCount; i++)
    if (geoDistanceM(ex.stops[i].latE6, ex.stops[i].lonE6, latE6, lonE6) < STOP_MIN_GAP_M)
      return STOP_TOO_CLOSE;
  PokeStop &s = ex.stops[ex.stopCount++];
  s = PokeStop();
  s.latE6 = latE6;
  s.lonE6 = lonE6;
  return STOP_OK;
}

StopResult stopRemove(Explore &ex, uint8_t idx) {
  if (idx >= ex.stopCount) return STOP_BAD_INDEX;
  for (int i = idx; i + 1 < ex.stopCount; i++) ex.stops[i] = ex.stops[i + 1];
  ex.stopCount--;
  return STOP_OK;
}

uint8_t stopNearest(const Explore &ex, int32_t latE6, int32_t lonE6, uint32_t *distM) {
  uint8_t best = 0xFF;
  uint32_t bestD = 0xFFFFFFFFu;
  for (int i = 0; i < ex.stopCount; i++) {
    uint32_t d = geoDistanceM(ex.stops[i].latE6, ex.stops[i].lonE6, latE6, lonE6);
    if (d < bestD) { bestD = d; best = (uint8_t)i; }
  }
  if (distM) *distM = bestD;
  return best;
}

StopResult stopSpin(Explore &ex, Trainer &t, Rng &rng, uint8_t idx, int32_t latE6, int32_t lonE6,
                    uint32_t now, Loot &out, uint16_t eggShinyDen) {
  if (idx >= ex.stopCount) return STOP_BAD_INDEX;
  PokeStop &s = ex.stops[idx];
  if (geoDistanceM(s.latE6, s.lonE6, latE6, lonE6) > STOP_SPIN_M) return STOP_TOO_FAR;
  if (now < s.readyAt) return STOP_COOLDOWN;
  s.readyAt = now + STOP_COOLDOWN_S;
  exploreLoot(ex, t, rng, out, eggShinyDen);
  return STOP_OK;
}

// ------------------------------------------------------------------ huevos --

bool eggAdd(Explore &ex, Rng &rng, uint8_t tier, uint16_t shinyDen) {
  if (tier < EGG_COMMON || tier > EGG_VERY_RARE) return false;
  for (int i = 0; i < MAX_EGGS; i++) {
    Egg &e = ex.eggs[i];
    if (e.species) continue;
    e = Egg();
    e.species = pickTierSpecies(rng, tier);
    e.tier = tier;
    e.shiny = rollOneIn(rng, shinyDen);
    return true;
  }
  return false;
}

bool eggReady(const Explore &ex, uint8_t idx) {
  if (idx >= MAX_EGGS || !ex.eggs[idx].species) return false;
  return ex.eggs[idx].walkedM >= (uint32_t)eggKm(ex.eggs[idx].tier) * 1000;
}

bool eggHatch(Explore &ex, Trainer &t, Rng &rng, uint8_t idx, OwnedMon *hatched) {
  if (!eggReady(ex, idx)) return false;
  Egg &e = ex.eggs[idx];
  int bl = partyBaseLevel(t);
  uint8_t level = (uint8_t)(bl - 5 > 5 ? bl - 5 : 5);
  OwnedMon m;
  ownedInit(m, e.species, level, rng);  // confianza 100: los huevos siempre obedecen
  if (e.shiny) m.flags |= OM_SHINY;
  if (!addOwned(t, m)) return false;  // equipo y PC llenos: el huevo espera
  dexMark(t.seen, e.species);
  dexMark(t.caught, e.species);
  if (hatched) *hatched = m;
  e = Egg();
  return true;
}

// ---------------------------------------------------------------- guardado -

#define EXPLORE_VERSION 1

size_t exploreSaveSize() { return SAVE_HEADER + sizeof(Explore); }

size_t exploreSave(const Explore &ex, uint8_t *buf, size_t cap) {
  return saveBlob("PTE1", EXPLORE_VERSION, &ex, sizeof(Explore), buf, cap);
}

bool exploreLoad(Explore &ex, const uint8_t *buf, size_t len) {
  return loadBlob("PTE1", EXPLORE_VERSION, &ex, sizeof(Explore), buf, len);
}
