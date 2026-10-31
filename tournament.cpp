#include "tournament.h"
#include "gyms.h"
#include "shop.h"

// ------------------------------------------------------------------ tablas --

const CupInfo CUPS[NUM_CUPS] = {
  { "Open Cup", "Copa Abierta", "Pewter", LV_SCALED, 0 },
  { "Little Cup", "Copa Peque\244a", "Cerulean", LV_FIXED, 20 },
  { "Level 30 Cup", "Copa Nivel 30", "Vermilion", LV_CAP, 30 },
  { "Flat 50 Cup", "Copa Nivel 50", "Celadon", LV_FIXED, 50 },
  { "Mono-Type Cup", "Copa Monotipo", "Saffron", LV_SCALED, 0 },
  { "No Items Cup", "Copa Sin Objetos", "Fuchsia", LV_SCALED, 0 },
  { "Random Cup", "Copa Aleatoria", "Cinnabar", LV_SCALED, 0 },
  { "Champion Cup", "Copa Campe\242n", "Indigo Plateau", LV_SCALED, 0 },
};

const char *cupName(uint8_t cup) {
  if (cup >= NUM_CUPS) return "";
  return gLang == LANG_ES ? CUPS[cup].es : CUPS[cup].en;
}

#define AI_ALWAYS_4 9
const TierInfo TOUR_TIERS[NUM_TOUR_TIERS] = {
  { "Rookie", "Novato", 1, -1, 500, 3, 1 },
  { "Great", "Super", 2, 0, 2000, 6, 2 },
  { "Ultra", "Ultra", 3, 1, 5000, 12, 4 },
  { "Master", "Master", 0, AI_ALWAYS_4, 10000, 25, 8 },
};

// tipos con al menos 3 lineas evolutivas para armar equipos de un tipo
#define NUM_MONO_TYPES 12
static const uint8_t MONO_TYPES[NUM_MONO_TYPES] = {
  T_NORMAL, T_FIRE, T_WATER, T_GRASS, T_ELECTRIC, T_POISON, T_GROUND, T_ROCK, T_BUG, T_FIGHTING,
  T_PSYCHIC, T_FLYING,
};
#define MONO_ANY 0xFE  // Mono-Type sin tipo fijado: vale cualquiera que compartan

static const uint8_t PRIZE_STONES[5] = { IT_FIRE_STONE, IT_WATER_STONE, IT_THUNDER_STONE,
                                         IT_LEAF_STONE, IT_MOON_STONE };

static uint32_t mix(uint32_t a, uint32_t b) {
  uint32_t h = a * 2654435761u ^ (b + 0x9E3779B9u + (a << 6) + (a >> 2));
  return h ? h : 1;
}

// la version de las tablas entra en la semilla: el Daily solo coincide con la misma version
#define DAILY_SEED_VERSION 0x50544B31u  // "PTK1"

// ----------------------------------------------------------------- basicos --

bool canStillEvolve(uint8_t species) {
  for (int i = 0; i < NUM_EVOS; i++) if (EVO_TBL[i].from == species) return true;
  return false;
}

bool hasType(uint8_t species, uint8_t type) {
  return BSPECIES[species].type1 == type || BSPECIES[species].type2 == type;
}

uint32_t tourFee(uint8_t tier, bool featured) {
  if (tier >= NUM_TOUR_TIERS) return 0;
  uint32_t fee = TOUR_TIERS[tier].fee;
  return featured ? fee * (100 - FEATURED_DISCOUNT) / 100 : fee;
}

Featured featuredCup(uint16_t week) {
  Rng rng;
  rng.seed(mix(week, 0xFEA7u));
  Featured f;
  f.cup = (uint8_t)rng.range(0, CUP_CHAMPION - 1);  // la Champion Cup no rota
  f.type = f.cup == CUP_MONO ? MONO_TYPES[rng.range(0, NUM_MONO_TYPES - 1)] : (uint8_t)T_NONE;
  return f;
}

DailyChallenge dailyChallenge(uint16_t day) {
  DailyChallenge d;
  d.rule = (uint8_t)(day % NUM_DAILY_RULES);
  d.type = d.rule == DR_MONO ? MONO_TYPES[(day / NUM_DAILY_RULES) % NUM_MONO_TYPES] : (uint8_t)T_NONE;
  // el Daily Prize rota: Caramelo Raro, piedra, Pepita, MT
  switch ((day / 7 + day) % 4) {
    case 0: d.prizeKind = PZ_ITEM; d.prizeId = IT_RARE_CANDY; break;
    case 1: d.prizeKind = PZ_ITEM; d.prizeId = PRIZE_STONES[(day / 4) % 5]; break;
    case 2: d.prizeKind = PZ_ITEM; d.prizeId = IT_NUGGET; break;
    default: {
      uint8_t tms[CELADON_TMS];
      celadonTms(day, tms);
      d.prizeKind = PZ_TM;
      d.prizeId = tms[0];
    }
  }
  return d;
}

static const OwnedMon &pickMon(const Trainer &t, const TourPick &p) {
  return p.inBox ? t.box[p.idx] : t.party[p.idx];
}
static OwnedMon &pickMon(Trainer &t, const TourPick &p) { return p.inBox ? t.box[p.idx] : t.party[p.idx]; }

// tipo que comparten los 3 (T_NONE si ninguno)
static uint8_t sharedType(const Trainer &t, const TourPick picks[TOUR_TEAM]) {
  const BattleSpecies &first = BSPECIES[pickMon(t, picks[0]).species];
  uint8_t cand[2] = { first.type1, first.type2 };
  for (uint8_t ty : cand) {
    if (ty == T_NONE) continue;
    bool all = true;
    for (int i = 1; i < TOUR_TEAM; i++) all &= hasType(pickMon(t, picks[i]).species, ty);
    if (all) return ty;
  }
  return T_NONE;
}

TourResult tourCheckTeam(const Trainer &t, const TourPick picks[TOUR_TEAM], bool little, uint8_t monoType) {
  for (int i = 0; i < TOUR_TEAM; i++) {
    const TourPick &p = picks[i];
    if (p.inBox ? p.idx >= t.boxCount : p.idx >= t.partyCount) return TOUR_BAD_PICK;
    for (int j = 0; j < i; j++)
      if (picks[j].inBox == p.inBox && picks[j].idx == p.idx) return TOUR_DUPLICATE;
    if (little && !canStillEvolve(pickMon(t, p).species)) return TOUR_RULE;
    if (monoType != T_NONE && monoType != MONO_ANY && !hasType(pickMon(t, p).species, monoType))
      return TOUR_RULE;
  }
  if (monoType == MONO_ANY && sharedType(t, picks) == T_NONE) return TOUR_RULE;
  return TOUR_OK;
}

static uint8_t clampLv(int v) { return (uint8_t)(v < 2 ? 2 : v > 100 ? 100 : v); }

static uint8_t entrantsBaseLevel(const Trainer &t, const TourPick picks[TOUR_TEAM]) {
  BattleMon tmp[TOUR_TEAM];
  for (int i = 0; i < TOUR_TEAM; i++) tmp[i].level = pickMon(t, picks[i]).level;
  return baseLevel(tmp, TOUR_TEAM);
}

// ----------------------------------------------------------- inscripcion ---

static void rollSpecial(Tournament &run, const Trainer &t, const Progress &p, const Featured &f, Rng &rng) {
  run.special = SPC_NONE;
  if (run.little) return;  // los retadores especiales no tienen equipos de Copa Pequena
  // evento de la semana: el lider del tipo destacado en el 25 % de las finales
  if (run.featured && f.type != T_NONE) {
    for (int g = 0; g < NUM_GYMS; g++)
      if (GYMS[g].type == f.type && rng.chance(25)) { run.special = SPC_GYM; run.specialIdx = (uint8_t)g; return; }
  }
  int r = rng.range(0, 99);
  if (r < 5) { run.special = SPC_BLUE; run.specialIdx = ELITE_CHAMPION; return; }
  if (r < 15 && t.badges) {
    uint8_t g;
    do g = (uint8_t)rng.range(0, NUM_GYMS - 1); while (!(t.badges & (1 << g)));
    run.special = SPC_GYM; run.specialIdx = g;
    return;
  }
  if (r < 17 && (p.eliteBeaten & 0x0F)) {
    uint8_t e;
    do e = (uint8_t)rng.range(0, 3); while (!(p.eliteBeaten & (1 << e)));
    run.special = SPC_ELITE; run.specialIdx = e;
  }
}

TourResult tourEnter(Tournament &run, Trainer &t, Progress &p, uint8_t cup, uint8_t tier,
                     const TourPick picks[TOUR_TEAM], uint32_t now, Rng &rng) {
  if (cup >= NUM_CUPS || tier >= NUM_TOUR_TIERS) return TOUR_RULE;
  if (cup == CUP_CHAMPION && (!t.champion || tier != TT_MASTER)) return TOUR_LOCKED;
  progressWeek(p, now);
  Featured f = featuredCup(weekIndex(now));

  Tournament r;
  r.cup = cup;
  r.tier = tier;
  r.featured = f.cup == cup;
  r.little = cup == CUP_LITTLE;
  r.noItems = cup == CUP_NO_ITEMS;

  if (cup == CUP_RANDOM) {
    // 3 prestados al azar del PC
    if (t.boxCount < TOUR_TEAM) return TOUR_NOT_ENOUGH;
    uint8_t idx[BOX_MAX];
    for (int i = 0; i < t.boxCount; i++) idx[i] = (uint8_t)i;
    for (int k = 0; k < TOUR_TEAM; k++) {
      int j = rng.range(k, t.boxCount - 1);
      uint8_t x = idx[k]; idx[k] = idx[j]; idx[j] = x;
      r.picks[k] = { 1, idx[k] };
    }
  } else {
    for (int i = 0; i < TOUR_TEAM; i++) r.picks[i] = picks[i];
    uint8_t mono = cup == CUP_MONO ? (r.featured ? f.type : (uint8_t)MONO_ANY) : (uint8_t)T_NONE;
    TourResult chk = tourCheckTeam(t, r.picks, r.little, mono);
    if (chk != TOUR_OK) return chk;
    if (cup == CUP_MONO) r.monoType = r.featured ? f.type : sharedType(t, r.picks);
  }

  r.fee = tourFee(tier, r.featured);
  if (!spend(t, r.fee)) return TOUR_NO_MONEY;

  const CupInfo &c = CUPS[cup];
  if (c.levelRule == LV_SCALED) r.oppLevel = clampLv(entrantsBaseLevel(t, r.picks) + TOUR_TIERS[tier].offset);
  else r.oppLevel = c.level;
  r.seed = rng.next();
  rollSpecial(r, t, p, f, rng);
  run = r;
  return TOUR_OK;
}

TourResult dailyEnter(Tournament &run, Trainer &t, Progress &p, Explore &ex,
                      const TourPick picks[TOUR_TEAM], uint32_t now, Rng &rng) {
  if (dailyPlayedToday(p, now)) return TOUR_DAILY_DONE;
  uint16_t day = dayIndex(now);
  DailyChallenge dc = dailyChallenge(day);
  Tournament r;
  r.daily = true;
  r.cup = CUP_FLAT50;
  r.tier = TT_GREAT;
  r.little = dc.rule == DR_LITTLE;
  r.noItems = dc.rule == DR_NO_ITEMS;
  r.monoType = dc.rule == DR_MONO ? dc.type : (uint8_t)T_NONE;
  for (int i = 0; i < TOUR_TEAM; i++) r.picks[i] = picks[i];
  TourResult chk = tourCheckTeam(t, r.picks, r.little, r.monoType);
  if (chk != TOUR_OK) return chk;
  r.oppLevel = DAILY_LEVEL;
  r.seed = mix(day, DAILY_SEED_VERSION);  // el mismo cuadro para todos ese dia
  dailyCountDay(p, t, ex, rng, now);       // el intento cuenta al empezar
  run = r;
  return TOUR_OK;
}

// ------------------------------------------------------------------ equipos -

static uint8_t playerLevel(const Tournament &run, uint8_t real) {
  if (run.daily) return DAILY_LEVEL;
  const CupInfo &c = CUPS[run.cup];
  if (c.levelRule == LV_FIXED) return c.level;
  if (c.levelRule == LV_CAP) return real > c.level ? c.level : real;
  return real;
}

void tourPlayerSide(const Tournament &run, const Trainer &t, BattleSide &side) {
  side = BattleSide();
  for (int i = 0; i < TOUR_TEAM; i++) {
    const OwnedMon &m = pickMon(t, run.picks[i]);
    BattleMon &b = side.party[i];
    monInit(b, m.species, playerLevel(run, m.level), m.genes, m.statExp);
    for (int k = 0; k < 4; k++) monSetMove(b, (uint8_t)k, m.moves[k]);  // PP al maximo
    b.trust = m.trust;
  }
  side.count = TOUR_TEAM;
}

uint8_t tourItemsAllowed(const Tournament &run) { return run.noItems ? 0 : TOUR_ITEMS; }

static Rng rivalRng(const Tournament &run, uint8_t round) {
  Rng rng;
  rng.seed(mix(run.seed, round + 1));
  return rng;
}

TourRival tourRival(const Tournament &run) {
  if (run.wins == TOUR_ROUNDS - 1 && run.special != SPC_NONE) return { run.special, run.specialIdx };
  Rng rng = rivalRng(run, run.wins);
  return { SPC_NONE, (uint8_t)rng.range(0, NUM_TRAINER_CLASSES - 1) };
}

#ifndef TOUR_SEXP_PCT
#define TOUR_SEXP_PCT 100
#endif
// los rivales de torneo son entrenadores con equipos entrenados: Stat Exp como
// la que junta un Pokemon del jugador peleando hasta ese nivel (nivel^2 * 8)
static void finishMon(BattleMon &m, uint8_t species, uint8_t lv, Rng &rng, uint8_t geneLo, uint8_t geneHi) {
  uint8_t genes[5];
  uint16_t sexp[5];
  uint32_t e = (uint32_t)lv * lv * 8 * TOUR_SEXP_PCT / 100;
  for (int k = 0; k < 5; k++) {
    genes[k] = (uint8_t)rng.range(geneLo, geneHi);
    sexp[k] = (uint16_t)(e > 65535 ? 65535 : e);
  }
  monInit(m, species, lv, genes, sexp);
}

static void buildSpecial(const Tournament &run, const BattleSide &player, BattleSide &opp) {
  Rng rng = rivalRng(run, 7);
  uint8_t list[8];
  uint8_t n = 0;
  if (run.special == SPC_GYM) {
    const GymInfo &g = GYMS[run.specialIdx];
    for (int i = 0; i < 8 && g.pool[i]; i++) list[n++] = g.pool[i];
  } else {
    const EliteInfo &e = ELITE[run.specialIdx];
    for (int i = 0; i < e.count && n < 8; i++) if (e.team[i]) list[n++] = e.team[i];
  }
  for (int k = 0; k < n; k++) {  // barajar
    int j = rng.range(k, n - 1);
    uint8_t x = list[k]; list[k] = list[j]; list[j] = x;
  }
  uint8_t lv = run.oppLevel;
  for (int i = 0; i < TOUR_TEAM; i++) {
    uint8_t base = list[i % n];
    if (i == TOUR_TEAM - 1 && run.special == SPC_GYM)  // el as del lider cierra
      base = lv >= 35 ? GYMS[run.specialIdx].aceHigh : GYMS[run.specialIdx].ace;
    finishMon(opp.party[i], evolveForLevel(base, lv), lv, rng, 105, 115);
    pickMovesSmart(opp.party[i], player.party, player.count, true);
  }
  opp.aiTier = 4;
}

void tourOpponent(const Tournament &run, const BattleSide &player, BattleSide &opp) {
  opp = BattleSide();
  opp.count = TOUR_TEAM;
  uint8_t lv = run.oppLevel;
  opp.potions = run.noItems ? 0 : (uint8_t)(run.tier == TT_ROOKIE ? 0 : run.tier == TT_GREAT ? 1 : 2);
  opp.potionHeal = potionForLevel(lv);

  TourRival who = tourRival(run);
  if (who.special != SPC_NONE) { buildSpecial(run, player, opp); return; }

  Rng rng = rivalRng(run, run.wins);
  rng.next();  // la primera tirada fue la clase (tourRival)
  // candidatos: formas base (con la forma que tendrian a ese nivel) que cumplen la copa
  uint8_t cand[151];
  uint8_t n = 0;
  for (int s = 1; s <= 151; s++) {
    if (!WILD_TIER[s]) continue;
    if (run.little && !canStillEvolve((uint8_t)s)) continue;
    uint8_t sp = run.little ? (uint8_t)s : evolveForLevel((uint8_t)s, lv);
    if (run.monoType != T_NONE && !hasType(sp, run.monoType)) continue;
    cand[n++] = (uint8_t)s;
  }
  if (!n) cand[n++] = 16;  // no pasa con MONO_TYPES, pero por si acaso
  bool scaled = !run.daily && CUPS[run.cup].levelRule == LV_SCALED;
  bool master = run.tier == TT_MASTER && !run.daily;
  for (int i = 0; i < TOUR_TEAM; i++) {
    int j = rng.range(i < n ? i : 0, n - 1);  // sin repetir mientras haya
    if (i < n) { uint8_t x = cand[i]; cand[i] = cand[j]; cand[j] = x; j = i; }
    uint8_t mlv = clampLv(scaled && i < TOUR_TEAM - 1 ? lv - 1 : lv);
    uint8_t sp = run.little ? cand[j] : evolveForLevel(cand[j], mlv);
    finishMon(opp.party[i], sp, mlv, rng, 90, 110);
    if (!run.daily && run.tier >= TT_ULTRA) pickMovesSmart(opp.party[i], player.party, player.count, master);
  }
  int ai = aiTierForLevel(lv);
  int8_t d = run.daily ? 0 : TOUR_TIERS[run.tier].aiDelta;
  ai = d == AI_ALWAYS_4 ? 4 : ai + d;
  opp.aiTier = (uint8_t)(ai < 1 ? 1 : ai > 4 ? 4 : ai);
}

// ------------------------------------------------------------- resultados --

void tourReport(Tournament &run, bool won, bool anyFainted, uint8_t hpPct, uint16_t turns) {
  if (run.finished) return;
  if (won) run.wins++;
  run.anyFainted |= anyFainted;
  run.score += (won ? 1000 : 0) + 5 * (hpPct > 100 ? 100 : hpPct) - 10 * turns;
  if (!won || run.wins >= TOUR_ROUNDS) run.finished = true;
}

bool tourOver(const Tournament &run) { return run.finished; }

static void givePrize(Trainer &t, uint8_t kind, uint8_t id) {
  if (kind == PZ_ITEM) addItem(t, id);
  else if (kind == PZ_TM) giveTm(t, id);
}

TourPayout tourFinish(Tournament &run, Trainer &t, Progress &p, Explore &ex, uint32_t now) {
  TourPayout out;
  run.finished = true;
  out.champion = run.wins == TOUR_ROUNDS;
  out.finalist = run.wins == TOUR_ROUNDS - 1;
  const TierInfo &ti = TOUR_TIERS[run.tier];

  if (run.daily) {
    // sin dinero; BP x2 de la categoria Great
    out.bp = (uint16_t)(out.champion ? ti.champBp * 2 : out.finalist ? ti.finBp * 2 : 0);
    out.perfect = out.champion && !run.anyFainted;
    if (out.perfect) { out.bp = out.bp * 3 / 2; p.perfectCrown = true; }
    p.dcScore = (uint16_t)(run.score < 0 ? 0 : run.score > 65535 ? 65535 : run.score);
    if (out.champion) {
      p.titles |= TITLE_DAILY_CHAMPION;
      DailyChallenge dc = dailyChallenge(dayIndex(now));
      out.prizeKind = dc.prizeKind;
      out.prizeId = dc.prizeId;
      if (dc.prizeKind == PZ_ITEM && dc.prizeId == IT_RARE_CANDY) {
        // entra en el tope diario de Caramelos gratis; si ya se gasto, una Pepita
        if (!freeRareCandy(ex, t, now)) { out.prizeId = IT_NUGGET; addItem(t, IT_NUGGET); }
      } else {
        givePrize(t, out.prizeKind, out.prizeId);
      }
    }
  } else {
    // ronda 1 pierde la cuota, semifinal la devuelve, final x1,5 y campeon x3
    if (out.champion) out.money = (uint32_t)ti.fee * 3;
    else if (out.finalist) out.money = (uint32_t)ti.fee * 3 / 2;
    else if (run.wins == 1) out.money = run.fee;
    out.bp = (uint16_t)(out.champion ? ti.champBp : out.finalist ? ti.finBp : 0);
    if (run.featured) out.bp *= 2;
  }
  out.specialBeaten = out.champion && run.special != SPC_NONE;
  if (out.specialBeaten) out.bp = out.bp * 3 / 2;

  earn(t, out.money);
  t.bp += out.bp;

  if (out.champion) {
    p.tournamentsWon++;
    for (int i = 0; i < TOUR_TEAM; i++) trustTournamentWin(pickMon(t, run.picks[i]));
    if (run.cup == CUP_MONO && run.monoType == T_FIRE && run.tier == TT_MASTER)
      p.titles |= TITLE_FIRE_CUP_MASTER;
    if (run.featured && !run.daily) {
      progressWeek(p, now);
      p.featuredWins++;
      if (p.featuredWins >= 3 && !p.weeklyPrizeGiven) {
        // premio semanal: MT que no se vende, Piedra Lunar o la cinta de la copa
        p.weeklyPrizeGiven = true;
        uint16_t w = weekIndex(now);
        switch (w % 3) {
          case 0: out.weeklyKind = PZ_TM; out.weeklyId = UNSOLD_TMS[(w / 3) % NUM_UNSOLD_TMS]; break;
          case 1: out.weeklyKind = PZ_ITEM; out.weeklyId = IT_MOON_STONE; break;
          default: out.weeklyKind = PZ_RIBBON; out.weeklyId = run.cup; p.ribbons |= (uint8_t)(1 << run.cup);
        }
        givePrize(t, out.weeklyKind, out.weeklyId);
      }
    }
  }
  return out;
}

ExpBonus tourExpBonus(const Tournament &run) {
  ExpBonus b = { true, false, run.daily || run.featured, false };
  return b;
}

uint8_t tourExpFoeLevel(const Tournament &run, uint8_t ownRealLevel, uint8_t foeLevel) {
  if (run.daily || CUPS[run.cup].levelRule != LV_SCALED) return ownRealLevel;
  return foeLevel;
}
