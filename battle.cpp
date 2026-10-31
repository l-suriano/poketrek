#include "battle.h"
#include <string.h>

// ids de PokeAPI de los movimientos con reglas propias
enum : uint8_t {
  MV_PAY_DAY = 6, MV_GUILLOTINE = 12, MV_RAZOR_WIND = 13, MV_WHIRLWIND = 18, MV_FLY = 19,
  MV_JUMP_KICK = 26, MV_HORN_DRILL = 32, MV_THRASH = 37, MV_ROAR = 46, MV_SONIC_BOOM = 49,
  MV_MIST = 54, MV_HYPER_BEAM = 63, MV_COUNTER = 68, MV_SEISMIC_TOSS = 69, MV_SOLAR_BEAM = 76,
  MV_PETAL_DANCE = 80, MV_DRAGON_RAGE = 82, MV_FISSURE = 90, MV_DIG = 91, MV_TOXIC = 92,
  MV_TELEPORT = 100, MV_NIGHT_SHADE = 101, MV_MIMIC = 102, MV_RECOVER = 105,
  MV_LIGHT_SCREEN = 113, MV_HAZE = 114, MV_REFLECT = 115, MV_FOCUS_ENERGY = 116, MV_BIDE = 117,
  MV_METRONOME = 118, MV_MIRROR_MOVE = 119, MV_SELF_DESTRUCT = 120, MV_SKULL_BASH = 130,
  MV_SOFT_BOILED = 135, MV_HI_JUMP_KICK = 136, MV_DREAM_EATER = 138, MV_SKY_ATTACK = 143,
  MV_TRANSFORM = 144, MV_PSYWAVE = 149, MV_SPLASH = 150, MV_EXPLOSION = 153, MV_REST = 156,
  MV_CONVERSION = 160, MV_SUPER_FANG = 162, MV_SUBSTITUTE = 164, MV_STRUGGLE = 165,
};

#define NO_SLOT 0xFF

// multiplicadores de etapa de gen 1 (-6..+6) en centesimas
static const uint16_t STAGE_MUL[13] = { 25, 28, 33, 40, 50, 66, 100, 150, 200, 250, 300, 350, 400 };

static inline uint32_t applyStage(uint32_t v, int8_t st) { return v * STAGE_MUL[st + 6] / 100; }
static inline bool physicalType(uint8_t t) { return t < T_FIRE; }  // Normal..Fantasma
static inline int foeOf(int s) { return s ^ 1; }

static uint32_t isqrtCeil(uint32_t v) {
  uint32_t r = 0;
  while ((r + 1) * (r + 1) <= v) r++;
  return r * r == v ? r : r + 1;
}

static void emit(Battle &b, uint8_t kind, int s, uint8_t a = 0, int16_t v = 0) {
  if (b.evCount < BATTLE_EVENTS) b.ev[b.evCount++] = { kind, (uint8_t)s, a, v };
}

// ---------------------------------------------------------------- Pokemon --

static uint16_t calcStat(uint8_t base, uint8_t level, uint16_t statExp, bool hp, uint8_t gene) {
  // formula de gen 1 con DV medio (8) y los genes de PokeTrek encima
  uint32_t v = ((uint32_t)(base + 8) * 2 + isqrtCeil(statExp) / 4) * level / 100;
  v = hp ? v + level + 10 : v + 5;
  v = v * gene / 100;
  return v < 1 ? 1 : (uint16_t)v;
}

void monInit(BattleMon &m, uint8_t species, uint8_t level, const uint8_t genes[5], const uint16_t statExp[5]) {
  static const uint8_t flat[5] = {100, 100, 100, 100, 100};
  static const uint16_t zero[5] = {0, 0, 0, 0, 0};
  if (!genes) genes = flat;
  if (!statExp) statExp = zero;
  const BattleSpecies &s = BSPECIES[species];
  m = BattleMon();
  m.species = species;
  m.level = level;
  m.type1 = s.type1;
  m.type2 = s.type2;
  m.maxHp = calcStat(s.hp, level, statExp[0], true, genes[0]) * HP_MULT;
  m.atk = calcStat(s.atk, level, statExp[1], false, genes[1]);
  m.def = calcStat(s.def, level, statExp[2], false, genes[2]);
  m.spe = calcStat(s.spe, level, statExp[3], false, genes[3]);
  m.spc = calcStat(s.spc, level, statExp[4], false, genes[4]);
  m.hp = m.maxHp;
  monDefaultMoves(m);
}

void monSetMove(BattleMon &m, uint8_t slot, uint8_t move) {
  if (slot >= 4) return;
  m.moves[slot] = move;
  m.pp[slot] = move ? MOVE_TBL[move].pp : 0;
}

void monDefaultMoves(BattleMon &m) {
  const BattleSpecies &s = BSPECIES[m.species];
  uint8_t list[4] = {0, 0, 0, 0};
  uint8_t n = 0;
  for (int i = 0; i < s.learnCount; i++) {
    const LearnEntry &e = LEARN_TBL[s.learnStart + i];
    if (e.level > m.level) continue;
    bool dup = false;
    for (int j = 0; j < n; j++) dup |= list[j] == e.move;
    if (dup) continue;
    if (n == 4) {  // como en gen 1: el nuevo empuja fuera al mas viejo
      memmove(list, list + 1, 3);
      n = 3;
    }
    list[n++] = e.move;
  }
  for (int i = 0; i < 4; i++) monSetMove(m, i, i < n ? list[i] : 0);
}

uint8_t evolveForLevel(uint8_t species, uint8_t level, uint8_t prefer) {
  for (int guard = 0; guard < 3; guard++) {
    uint8_t next = 0;
    for (int i = 0; i < NUM_EVOS; i++) {
      const EvoEntry &e = EVO_TBL[i];
      if (e.from != species) continue;
      bool ok = (e.method == EVO_LEVEL && level >= e.param) ||
                (e.method == EVO_STONE && level >= 30) ||
                (e.method == EVO_TRADE && level >= 35);
      if (!ok) continue;
      if (!next || (prefer != T_NONE && BSPECIES[e.to].type1 == prefer)) next = e.to;
    }
    if (!next) break;
    species = next;
  }
  return species;
}

// ---------------------------------------------------------------- campo ----

void battleInit(Battle &b, uint32_t seed, bool wild) {
  b.rng.seed(seed);
  b.wild = wild;
  b.outcome = OUT_NONE;
  b.turn = 0;
  b.evCount = 0;
  for (int s = 0; s < 2; s++) {
    b.side[s].vol = Volatile();
    b.side[s].hasSaved = false;
    for (int i = 0; i < b.side[s].count; i++) {
      if (b.side[s].party[i].hp) { b.side[s].active = i; break; }
    }
  }
}

BattleMon &activeMon(Battle &b, int s) { return b.side[s].party[b.side[s].active]; }
const BattleMon &activeMon(const Battle &b, int s) { return b.side[s].party[b.side[s].active]; }

static void restoreSaved(BattleSide &sd) {
  if (!sd.hasSaved) return;
  BattleMon &m = sd.party[sd.active];
  uint16_t hp = m.hp;
  uint8_t st = m.status, sl = m.sleepTurns, tox = m.toxicN;
  m = sd.saved;
  m.hp = hp > m.maxHp ? m.maxHp : hp;
  m.status = st; m.sleepTurns = sl; m.toxicN = tox;
  sd.hasSaved = false;
}

static void saveBeforeChange(BattleSide &sd) {
  if (sd.hasSaved) return;
  sd.saved = sd.party[sd.active];
  sd.hasSaved = true;
}

void battleSwitchIn(Battle &b, int s, uint8_t idx) {
  BattleSide &sd = b.side[s];
  if (idx >= sd.count || !sd.party[idx].hp) return;
  restoreSaved(sd);
  BattleMon &out = sd.party[sd.active];
  if (out.status == STS_TOX) out.status = STS_PSN;  // gen 1: Toxico vuelve a veneno normal al salir
  sd.active = idx;
  sd.vol = Volatile();
  emit(b, EV_SWITCH, s, idx);
}

void battleEnd(Battle &b) {
  for (int s = 0; s < 2; s++) {
    restoreSaved(b.side[s]);
    b.side[s].vol = Volatile();
  }
}

bool battleNeedsSwitch(const Battle &b, int s) {
  const BattleSide &sd = b.side[s];
  if (sd.party[sd.active].hp) return false;
  for (int i = 0; i < sd.count; i++) if (sd.party[i].hp) return true;
  return false;
}

static bool sideAlive(const BattleSide &sd) {
  for (int i = 0; i < sd.count; i++) if (sd.party[i].hp) return true;
  return false;
}

static void checkOutcome(Battle &b) {
  if (b.outcome) return;
  bool a0 = sideAlive(b.side[0]), a1 = sideAlive(b.side[1]);
  if (!a0) b.outcome = OUT_WIN1;  // si caen los dos (Explosion), pierde el jugador
  else if (!a1) b.outcome = OUT_WIN0;
}

bool canUseMove(const Battle &b, int s, uint8_t slot) {
  const BattleSide &sd = b.side[s];
  const BattleMon &m = sd.party[sd.active];
  if (slot >= 4 || !m.moves[slot] || !m.pp[slot]) return false;
  return !(sd.vol.disabledTurns && sd.vol.disabledMove == m.moves[slot]);
}

static bool anyMoveUsable(const Battle &b, int s) {
  for (int i = 0; i < 4; i++) if (canUseMove(b, s, i)) return true;
  return false;
}

// ---------------------------------------------------------------- dano -----

static inline bool hasType(const BattleMon &m, uint8_t t) { return m.type1 == t || m.type2 == t; }

static uint16_t effectiveSpeed(const BattleMon &m, const Volatile &v) {
  uint32_t sp = applyStage(m.spe, v.stage[SG_SPE]);
  if (m.status == STS_PAR) sp /= 4;
  return sp ? sp : 1;
}

static bool rollCrit(Battle &b, const BattleMon &m, const Volatile &v, uint8_t move) {
  uint32_t t = BSPECIES[m.species].spe / 2;
  if (MOVE_TBL[move].highCrit) t *= 8;
  if (v.focus) t *= 4;
  if (t > 255) t = 255;
  return (b.rng.next() & 0xFF) < t;
}

// dano de gen 1. eff2 devuelve la eficacia x2 (0 inmune .. 8 = x4)
static uint16_t calcDamage(const BattleMon &att, const Volatile &av, const BattleMon &def,
                           const Volatile &dv, uint8_t move, bool crit, uint8_t rnd, uint8_t *eff2) {
  const MoveInfo &mi = MOVE_TBL[move];
  uint8_t eff = typeEff2(mi.type, def.type1, def.type2);
  if (eff2) *eff2 = eff;
  if (!eff || !mi.power) return 0;
  bool phys = physicalType(mi.type);
  uint32_t A = phys ? att.atk : att.spc;
  uint32_t D = phys ? def.def : def.spc;
  if (!crit) {
    A = applyStage(A, av.stage[phys ? SG_ATK : SG_SPC]);
    D = applyStage(D, dv.stage[phys ? SG_DEF : SG_SPC]);
    if (phys && att.status == STS_BRN) A /= 2;
    if ((phys && dv.reflect) || (!phys && dv.lightScreen)) D *= 2;
  }
  if (A > 255 || D > 255) { A /= 4; D /= 4; }  // gen 1 trabaja con bytes
  if (!A) A = 1;
  if (!D) D = 1;
  uint32_t lvl = (uint32_t)att.level * (crit ? 2 : 1);
  uint32_t dmg = ((2 * lvl / 5 + 2) * mi.power * A / D) / 50;
  if (dmg > 997) dmg = 997;
  dmg += 2;
  if (hasType(att, mi.type)) dmg = dmg * 3 / 2;
  dmg = dmg * TYPE_EFF[mi.type][def.type1] / 2;
  if (def.type2 != T_NONE) dmg = dmg * TYPE_EFF[mi.type][def.type2] / 2;
  if (dmg > 1) dmg = dmg * rnd / 255;
  return dmg ? (uint16_t)dmg : 1;
}

uint16_t estimateDamage(const BattleMon &att, const BattleMon &def, uint8_t move) {
  Volatile none;
  return calcDamage(att, none, def, none, move, false, 236, nullptr);
}

static uint16_t fixedDamage(Battle &b, const BattleMon &att, const BattleMon &def, uint8_t move) {
  switch (move) {
    case MV_SONIC_BOOM: return 20;
    case MV_DRAGON_RAGE: return 40;
    case MV_SEISMIC_TOSS: case MV_NIGHT_SHADE: return att.level;
    case MV_PSYWAVE: return (uint16_t)b.rng.range(1, att.level * 3 / 2 > 1 ? att.level * 3 / 2 : 1);
    case MV_SUPER_FANG: return def.hp / 2 ? def.hp / 2 : 1;
    default: return 0;
  }
}

static bool isFixedDamage(uint8_t move) {
  return move == MV_SONIC_BOOM || move == MV_DRAGON_RAGE || move == MV_SEISMIC_TOSS ||
         move == MV_NIGHT_SHADE || move == MV_PSYWAVE || move == MV_SUPER_FANG;
}

static bool isTwoTurn(uint8_t move) {
  return move == MV_RAZOR_WIND || move == MV_SOLAR_BEAM || move == MV_SKULL_BASH ||
         move == MV_SKY_ATTACK || move == MV_FLY || move == MV_DIG;
}

// quita PS (respetando el Sustituto) y apunta el dano para Contraataque y Venganza
static uint16_t dealDamage(Battle &b, int target, uint16_t dmg, uint8_t moveType, bool viaSub) {
  BattleSide &sd = b.side[target];
  BattleMon &m = sd.party[sd.active];
  if (viaSub && sd.vol.substituteHp) {
    if (dmg >= sd.vol.substituteHp) sd.vol.substituteHp = 0;
    else sd.vol.substituteHp -= dmg;
    emit(b, EV_DAMAGE, target, 1, (int16_t)dmg);  // a = 1: lo recibio el sustituto
    return 0;
  }
  if (dmg > m.hp) dmg = m.hp;
  m.hp -= dmg;
  sd.vol.lastDamage = dmg;
  sd.vol.lastDamageType = moveType;
  if (sd.vol.bideTurns) sd.vol.bideDamage += dmg;
  emit(b, EV_DAMAGE, target, 0, (int16_t)dmg);
  if (!m.hp) {
    m.status = STS_NONE;
    emit(b, EV_FAINT, target);
  }
  return dmg;
}

static void heal(Battle &b, int s, uint16_t amount) {
  BattleMon &m = activeMon(b, s);
  uint16_t room = m.maxHp - m.hp;
  if (amount > room) amount = room;
  if (!amount) return;
  m.hp += amount;
  emit(b, EV_HEAL, s, 0, (int16_t)amount);
}

// ---------------------------------------------------------------- efectos --

static bool applyStat(Battle &b, int target, uint8_t stat, int8_t delta, bool fromFoe) {
  if (stat == ST_NONE || !delta) return false;
  Volatile &v = b.side[target].vol;
  if (fromFoe && delta < 0 && (v.mist || v.substituteHp)) return false;
  uint8_t idx = stat - ST_ATTACK;  // BStat empieza en ATTACK = 1
  int nv = v.stage[idx] + delta;
  if (nv > 6) nv = 6;
  if (nv < -6) nv = -6;
  if (nv == v.stage[idx]) return false;
  v.stage[idx] = (int8_t)nv;
  emit(b, EV_STAT, target, stat, delta);
  return true;
}

static bool applyAilment(Battle &b, int target, uint8_t ail, uint8_t move, bool secondary) {
  BattleSide &sd = b.side[target];
  BattleMon &m = sd.party[sd.active];
  const MoveInfo &mi = MOVE_TBL[move];
  if (!m.hp) return false;
  if (!secondary && sd.vol.substituteHp) return false;
  // un efecto secundario no afecta a quien comparte tipo con el ataque (gen 1)
  if (secondary && hasType(m, mi.type)) return false;
  switch (ail) {
    case AIL_PARALYSIS: case AIL_SLEEP: case AIL_FREEZE: case AIL_BURN: case AIL_POISON: {
      if (m.status) return false;
      if (ail == AIL_POISON && hasType(m, T_POISON)) return false;
      if (ail == AIL_BURN && hasType(m, T_FIRE)) return false;
      if (ail == AIL_FREEZE && hasType(m, T_ICE)) return false;
      if (!secondary && !typeEff2(mi.type, m.type1, m.type2)) return false;  // Onda Trueno a Tierra
      if (ail == AIL_SLEEP) { m.status = STS_SLP; m.sleepTurns = (uint8_t)b.rng.range(1, 7); }
      else if (ail == AIL_PARALYSIS) m.status = STS_PAR;
      else if (ail == AIL_FREEZE) m.status = STS_FRZ;
      else if (ail == AIL_BURN) m.status = STS_BRN;
      else if (move == MV_TOXIC) { m.status = STS_TOX; m.toxicN = 1; }
      else m.status = STS_PSN;
      emit(b, EV_STATUS, target, m.status);
      return true;
    }
    case AIL_CONFUSION:
      if (sd.vol.confused) return false;
      sd.vol.confused = (uint8_t)b.rng.range(2, 5);
      emit(b, EV_STATUS, target, 0x80);  // 0x80 = confusion
      return true;
    case AIL_TRAP:
      if (sd.vol.trapTurns) return false;
      sd.vol.trapTurns = (uint8_t)b.rng.range(2, 5);
      emit(b, EV_EFFECT, target, AIL_TRAP);
      return true;
    case AIL_LEECH_SEED:
      if (sd.vol.seeded || hasType(m, T_GRASS)) return false;
      sd.vol.seeded = true;
      emit(b, EV_EFFECT, target, AIL_LEECH_SEED);
      return true;
    case AIL_DISABLE: {
      if (sd.vol.disabledTurns) return false;
      uint8_t opts[4], n = 0;
      for (int i = 0; i < 4; i++) if (m.moves[i] && m.pp[i]) opts[n++] = m.moves[i];
      if (!n) return false;
      sd.vol.disabledMove = opts[b.rng.range(0, n - 1)];
      sd.vol.disabledTurns = (uint8_t)b.rng.range(1, 8);
      emit(b, EV_EFFECT, target, AIL_DISABLE, sd.vol.disabledMove);
      return true;
    }
    default:
      return false;
  }
}

static bool accuracyHits(Battle &b, int s, uint8_t move) {
  const MoveInfo &mi = MOVE_TBL[move];
  const Volatile &av = b.side[s].vol, &dv = b.side[foeOf(s)].vol;
  if (dv.invulnerable) return false;
  if (!mi.accuracy) return true;  // Rapidez y compania
  int8_t net = av.stage[SG_ACC] - dv.stage[SG_EVA];
  if (net > 6) net = 6;
  if (net < -6) net = -6;
  uint32_t acc = applyStage(mi.accuracy, net);
  if (acc >= 100) return true;  // sin el fallo de 1/256 de gen 1
  return b.rng.chance((int)acc);
}

static uint8_t multiHitCount(Battle &b, const MoveInfo &mi) {
  if (!mi.maxHits) return 1;
  if (mi.minHits == mi.maxHits) return mi.maxHits;
  // 2-5 golpes con el reparto de gen 1: 37.5 / 37.5 / 12.5 / 12.5 %
  int r = b.rng.range(0, 7);
  return r < 3 ? 2 : r < 6 ? 3 : r < 7 ? 4 : 5;
}

static void hurtSelfConfused(Battle &b, int s) {
  BattleMon &m = activeMon(b, s);
  Volatile &v = b.side[s].vol;
  uint32_t A = applyStage(m.atk, v.stage[SG_ATK]), D = applyStage(m.def, v.stage[SG_DEF]);
  if (!D) D = 1;
  uint32_t dmg = ((2 * m.level / 5 + 2) * 40 * A / D) / 50 + 2;
  emit(b, EV_CONFUSED_HIT, s);
  dealDamage(b, s, (uint16_t)dmg, T_NORMAL, false);
}

// ---------------------------------------------------------------- turno ----

static void useMove(Battle &b, int s, uint8_t move, uint8_t slot);

// devuelve false si el Pokemon no llega a actuar este turno
static bool canAct(Battle &b, int s, uint8_t &move, uint8_t &slot) {
  BattleSide &sd = b.side[s];
  BattleMon &m = sd.party[sd.active];
  Volatile &v = sd.vol;
  if (v.recharge) { v.recharge = false; emit(b, EV_RECHARGE, s); return false; }
  if (m.status == STS_SLP) {
    if (m.sleepTurns) m.sleepTurns--;
    if (!m.sleepTurns) { m.status = STS_NONE; emit(b, EV_WAKE, s); }
    else emit(b, EV_ASLEEP, s);
    return false;  // en gen 1 despertar gasta el turno
  }
  if (m.status == STS_FRZ) {
    if (b.rng.chance(20)) { m.status = STS_NONE; emit(b, EV_THAW, s); }
    else { emit(b, EV_FROZEN, s); return false; }
  }
  if (v.flinch) { v.flinch = false; emit(b, EV_FLINCH, s); return false; }
  // desobediencia: solo al escoger un movimiento nuevo, no en los forzados
  if (slot != NO_SLOT && m.trust < 100 && b.rng.chance((100 - m.trust) / 2)) {
    int r = b.rng.range(0, 99);
    emit(b, EV_DISOBEY, s, (uint8_t)(r < 50 ? 0 : r < 80 ? 1 : r < 95 ? 2 : 3));
    if (r < 50) return false;                                  // se pone a holgazanear
    if (r < 80) {                                              // usa otro movimiento
      uint8_t opts[4], n = 0;
      for (int i = 0; i < 4; i++) if (i != slot && canUseMove(b, s, i)) opts[n++] = i;
      if (n) { slot = opts[b.rng.range(0, n - 1)]; move = m.moves[slot]; }
    } else if (r < 95) {                                       // se echa a dormir
      if (!m.status) { m.status = STS_SLP; m.sleepTurns = (uint8_t)b.rng.range(1, 3); emit(b, EV_STATUS, s, STS_SLP); }
      return false;
    } else {                                                   // se hace dano
      hurtSelfConfused(b, s);
      return false;
    }
  }
  if (v.confused) {
    v.confused--;
    if (!v.confused) emit(b, EV_EFFECT, s, AIL_CONFUSION, 0);  // se le pasa
    else if (b.rng.chance(50)) { hurtSelfConfused(b, s); return false; }
  }
  if (m.status == STS_PAR && b.rng.chance(25)) { emit(b, EV_FULL_PARA, s); return false; }
  return true;
}

static void doDamagingMove(Battle &b, int s, uint8_t move) {
  BattleSide &as = b.side[s], &ds = b.side[foeOf(s)];
  BattleMon &att = as.party[as.active];
  BattleMon &def = ds.party[ds.active];
  const MoveInfo &mi = MOVE_TBL[move];

  if (move == MV_DREAM_EATER && def.status != STS_SLP) { emit(b, EV_FAIL, s); return; }
  if (mi.category == MC_OHKO) {
    if (att.spe < def.spe || !typeEff2(mi.type, def.type1, def.type2)) { emit(b, EV_FAIL, s); return; }
    dealDamage(b, foeOf(s), def.hp, mi.type, true);
    emit(b, EV_EFFECT, s, 0xF0);  // 0xF0 = KO de un golpe
    return;
  }

  uint16_t dmg;
  uint8_t eff = 2;
  bool crit = false;
  if (isFixedDamage(move)) {
    dmg = fixedDamage(b, att, def, move);
  } else if (move == MV_COUNTER) {
    bool ok = as.vol.lastDamage && physicalType(as.vol.lastDamageType) &&
              (as.vol.lastDamageType == T_NORMAL || as.vol.lastDamageType == T_FIGHTING);
    if (!ok) { emit(b, EV_FAIL, s); return; }
    dmg = as.vol.lastDamage * 2;
  } else {
    crit = rollCrit(b, att, as.vol, move);
    dmg = calcDamage(att, as.vol, def, ds.vol, move, crit, (uint8_t)b.rng.range(217, 255), &eff);
    if (!eff) { emit(b, EV_EFFECT, s, 0xF1); return; }  // 0xF1 = no afecta
  }

  uint8_t hits = multiHitCount(b, mi);
  uint16_t total = 0;
  for (uint8_t h = 0; h < hits && def.hp; h++) {
    if (crit) emit(b, EV_CRIT, s);
    total += dealDamage(b, foeOf(s), dmg, mi.type, true);
  }
  if (eff != 2 && !isFixedDamage(move)) emit(b, EV_EFFECT, s, 0xF2, eff);  // eficacia para la UI

  // drenaje y retroceso
  if (move == MV_STRUGGLE) {
    dealDamage(b, s, total / 2 ? total / 2 : 1, T_NORMAL, false);
  } else if (mi.drain > 0 && total) {
    heal(b, s, (uint16_t)(total * mi.drain / 100 ? total * mi.drain / 100 : 1));
  } else if (mi.drain < 0 && total) {
    uint16_t r = (uint16_t)(total * (-mi.drain) / 100);
    dealDamage(b, s, r ? r : 1, T_NORMAL, false);
  }

  if (move == MV_HYPER_BEAM && def.hp) as.vol.recharge = true;  // si debilita, no recarga (gen 1)

  // efectos secundarios
  if (def.hp && !ds.vol.substituteHp) {
    bool roll = !mi.chance || b.rng.chance(mi.chance);
    if (mi.ailment != AIL_NONE && roll) {
      applyAilment(b, foeOf(s), mi.ailment, move, mi.ailment != AIL_TRAP);
    }
    if (mi.stat1 != ST_NONE && (mi.chance ? b.rng.chance(mi.chance) : true)) {
      int t = mi.selfStat ? s : foeOf(s);
      applyStat(b, t, mi.stat1, mi.delta1, !mi.selfStat);
      if (mi.stat2 != ST_NONE) applyStat(b, t, mi.stat2, mi.delta2, !mi.selfStat);
    }
    if (mi.flinch && b.rng.chance(mi.flinch)) ds.vol.flinch = true;  // solo cuenta si aun no actuo
  }
}

static void doStatusMove(Battle &b, int s, uint8_t move) {
  const MoveInfo &mi = MOVE_TBL[move];
  bool did = false;
  if (mi.ailment != AIL_NONE) did |= applyAilment(b, foeOf(s), mi.ailment, move, false);
  if (mi.stat1 != ST_NONE) {
    int t = mi.selfStat ? s : foeOf(s);
    did |= applyStat(b, t, mi.stat1, mi.delta1, !mi.selfStat);
    if (mi.stat2 != ST_NONE) did |= applyStat(b, t, mi.stat2, mi.delta2, !mi.selfStat);
  }
  if (!did) emit(b, EV_FAIL, s);
}

static bool needsAccuracy(uint8_t move) {
  const MoveInfo &mi = MOVE_TBL[move];
  if (mi.category == MC_NET_GOOD_STATS && mi.selfStat) return false;
  if (mi.category == MC_HEAL || mi.category == MC_FIELD_EFFECT || mi.category == MC_WHOLE_FIELD_EFFECT) return false;
  switch (move) {
    case MV_BIDE: case MV_METRONOME: case MV_MIRROR_MOVE: case MV_SUBSTITUTE: case MV_SPLASH:
    case MV_FOCUS_ENERGY: case MV_CONVERSION: case MV_TRANSFORM: case MV_TELEPORT: case MV_REST:
      return false;
    default:
      return true;
  }
}

static void useMove(Battle &b, int s, uint8_t move, uint8_t slot) {
  BattleSide &as = b.side[s], &ds = b.side[foeOf(s)];
  BattleMon &att = as.party[as.active];
  BattleMon &def = ds.party[ds.active];
  Volatile &v = as.vol;
  const MoveInfo &mi = MOVE_TBL[move];

  if (slot != NO_SLOT && slot < 4 && att.pp[slot]) att.pp[slot]--;
  emit(b, EV_MOVE, s, move);
  if (move != MV_MIRROR_MOVE) v.lastMove = move;

  // movimientos de dos turnos: el primero solo carga
  if (isTwoTurn(move) && v.charging != move) {
    v.charging = move;
    v.invulnerable = (move == MV_FLY || move == MV_DIG);
    emit(b, EV_CHARGE, s, move);
    return;
  }
  if (v.charging == move) { v.charging = 0; v.invulnerable = false; }

  // Venganza: aguanta 2-3 turnos y devuelve el doble
  if (move == MV_BIDE) {
    if (!v.bideTurns) { v.bideTurns = (uint8_t)b.rng.range(2, 3); v.bideDamage = 0; emit(b, EV_CHARGE, s, move); return; }
    if (--v.bideTurns) { emit(b, EV_CHARGE, s, move); return; }
    if (!v.bideDamage) { emit(b, EV_FAIL, s); return; }
    dealDamage(b, foeOf(s), v.bideDamage * 2, T_NORMAL, true);
    return;
  }

  // movimientos que llaman a otros
  if (move == MV_METRONOME) {
    uint8_t pick;
    do { pick = (uint8_t)b.rng.range(1, NUM_MOVES); } while (pick == MV_METRONOME || pick == MV_STRUGGLE);
    useMove(b, s, pick, NO_SLOT);
    return;
  }
  if (move == MV_MIRROR_MOVE) {
    uint8_t m = ds.vol.lastMove;
    if (!m || m == MV_MIRROR_MOVE) { emit(b, EV_FAIL, s); return; }
    useMove(b, s, m, NO_SLOT);
    return;
  }

  if (needsAccuracy(move) && !accuracyHits(b, s, move)) {
    emit(b, EV_MISS, s);
    if (move == MV_JUMP_KICK || move == MV_HI_JUMP_KICK) dealDamage(b, s, 1, T_FIGHTING, false);  // gen 1
    if (move == MV_SELF_DESTRUCT || move == MV_EXPLOSION) dealDamage(b, s, att.hp, T_NORMAL, false);
    if (v.lockTurns) { v.lockTurns = 0; v.lockMove = 0; }
    return;
  }

  switch (move) {
    case MV_SPLASH:  // Salpicadura no hace nada
      emit(b, EV_FAIL, s);
      return;
    case MV_TELEPORT: case MV_ROAR: case MV_WHIRLWIND:
      if (b.wild) { b.outcome = OUT_FLED; emit(b, EV_EFFECT, s, 0xF3); }
      else emit(b, EV_FAIL, s);
      return;
    case MV_FOCUS_ENERGY:
      if (v.focus) emit(b, EV_FAIL, s); else { v.focus = true; emit(b, EV_EFFECT, s, MV_FOCUS_ENERGY); }
      return;
    case MV_REFLECT:
      if (v.reflect) emit(b, EV_FAIL, s); else { v.reflect = true; emit(b, EV_EFFECT, s, MV_REFLECT); }
      return;
    case MV_LIGHT_SCREEN:
      if (v.lightScreen) emit(b, EV_FAIL, s); else { v.lightScreen = true; emit(b, EV_EFFECT, s, MV_LIGHT_SCREEN); }
      return;
    case MV_MIST:
      if (v.mist) emit(b, EV_FAIL, s); else { v.mist = true; emit(b, EV_EFFECT, s, MV_MIST); }
      return;
    case MV_HAZE:
      for (int t = 0; t < 2; t++) {
        Volatile &x = b.side[t].vol;
        memset(x.stage, 0, sizeof(x.stage));
        x.confused = 0; x.seeded = false; x.focus = false;
        x.reflect = x.lightScreen = x.mist = false;
      }
      emit(b, EV_EFFECT, s, MV_HAZE);
      return;
    case MV_RECOVER: case MV_SOFT_BOILED:
      if (att.hp == att.maxHp) { emit(b, EV_FAIL, s); return; }
      heal(b, s, att.maxHp / 2);
      return;
    case MV_REST:
      if (att.hp == att.maxHp) { emit(b, EV_FAIL, s); return; }
      heal(b, s, att.maxHp);
      att.status = STS_SLP; att.sleepTurns = 2; att.toxicN = 0;
      emit(b, EV_STATUS, s, STS_SLP);
      return;
    case MV_SUBSTITUTE: {
      uint16_t cost = att.maxHp / 4;
      if (v.substituteHp || att.hp <= cost) { emit(b, EV_FAIL, s); return; }
      att.hp -= cost;
      v.substituteHp = cost + 1;
      emit(b, EV_EFFECT, s, MV_SUBSTITUTE);
      return;
    }
    case MV_CONVERSION:
      saveBeforeChange(as);
      att.type1 = def.type1; att.type2 = def.type2;
      emit(b, EV_EFFECT, s, MV_CONVERSION);
      return;
    case MV_TRANSFORM: {
      if (v.transformed) { emit(b, EV_FAIL, s); return; }
      saveBeforeChange(as);
      uint16_t hp = att.hp, maxHp = att.maxHp;
      uint8_t lvl = att.level, st = att.status, sl = att.sleepTurns, tr = att.trust;
      att = def;
      att.hp = hp; att.maxHp = maxHp; att.level = lvl;
      att.status = st; att.sleepTurns = sl; att.trust = tr;
      for (int i = 0; i < 4; i++) att.pp[i] = att.moves[i] ? 5 : 0;
      memcpy(v.stage, ds.vol.stage, sizeof(v.stage));
      v.transformed = true;
      emit(b, EV_EFFECT, s, MV_TRANSFORM);
      return;
    }
    case MV_MIMIC: {
      uint8_t opts[4], n = 0;
      for (int i = 0; i < 4; i++) if (def.moves[i]) opts[n++] = def.moves[i];
      if (!n || slot == NO_SLOT) { emit(b, EV_FAIL, s); return; }
      saveBeforeChange(as);
      att.moves[slot] = opts[b.rng.range(0, n - 1)];
      emit(b, EV_EFFECT, s, MV_MIMIC, att.moves[slot]);
      return;
    }
    default:
      break;
  }

  // Golpe / Danza Petalo: 2-3 turnos seguidos y luego confusion
  if ((move == MV_THRASH || move == MV_PETAL_DANCE) && !v.lockTurns) {
    v.lockMove = move;
    v.lockTurns = (uint8_t)b.rng.range(2, 3);
  }

  if (mi.power || isFixedDamage(move) || move == MV_COUNTER || mi.category == MC_OHKO) {
    doDamagingMove(b, s, move);
  } else {
    doStatusMove(b, s, move);
  }

  if (move == MV_SELF_DESTRUCT || move == MV_EXPLOSION) dealDamage(b, s, att.hp, T_NORMAL, false);

  if (v.lockTurns && --v.lockTurns == 0) {
    v.lockMove = 0;
    if (att.hp && !v.confused) { v.confused = (uint8_t)b.rng.range(2, 5); emit(b, EV_STATUS, s, 0x80); }
  }
}

static void struggle(Battle &b, int s) { useMove(b, s, MV_STRUGGLE, NO_SLOT); }

static void runMoveAction(Battle &b, int s, uint8_t slot) {
  BattleSide &sd = b.side[s];
  BattleMon &m = sd.party[sd.active];
  Volatile &v = sd.vol;
  // acciones forzadas: carga, Golpe, Venganza
  uint8_t move;
  uint8_t useSlot = slot;
  if (v.charging) { move = v.charging; useSlot = NO_SLOT; }
  else if (v.lockTurns) { move = v.lockMove; useSlot = NO_SLOT; }
  else if (v.bideTurns) { move = MV_BIDE; useSlot = NO_SLOT; }
  else if (slot >= 4 || !canUseMove(b, s, slot)) {
    if (!anyMoveUsable(b, s)) {
      move = MV_STRUGGLE;
      useSlot = NO_SLOT;
      if (canAct(b, s, move, useSlot)) struggle(b, s);
      return;
    }
    for (slot = 0; slot < 4 && !canUseMove(b, s, slot); slot++) {}
    move = m.moves[slot];
    useSlot = slot;
  } else {
    move = m.moves[slot];
  }
  if (!canAct(b, s, move, useSlot)) {
    // si algo interrumpe, se cancela la carga o el bloqueo
    v.charging = 0; v.invulnerable = false;
    if (m.hp == 0) { v.lockTurns = 0; v.bideTurns = 0; }
    return;
  }
  useMove(b, s, move, useSlot);
}

static void endOfTurn(Battle &b, int s) {
  BattleSide &sd = b.side[s];
  BattleMon &m = sd.party[sd.active];
  Volatile &v = sd.vol;
  if (!m.hp) return;
  if (m.status == STS_BRN || m.status == STS_PSN) {
    dealDamage(b, s, m.maxHp / 16 ? m.maxHp / 16 : 1, T_NORMAL, false);
  } else if (m.status == STS_TOX) {
    uint16_t d = (uint16_t)((uint32_t)m.maxHp * m.toxicN / 16);
    if (m.toxicN < 15) m.toxicN++;
    dealDamage(b, s, d ? d : 1, T_POISON, false);
  }
  if (m.hp && v.seeded) {
    uint16_t d = m.maxHp / 16 ? m.maxHp / 16 : 1;
    d = dealDamage(b, s, d, T_GRASS, false);
    if (activeMon(b, foeOf(s)).hp) heal(b, foeOf(s), d);
  }
  if (m.hp && v.trapTurns) {
    v.trapTurns--;
    dealDamage(b, s, m.maxHp / 8 ? m.maxHp / 8 : 1, T_NORMAL, false);
  }
  if (v.disabledTurns && --v.disabledTurns == 0) v.disabledMove = 0;
  v.flinch = false;
}

void battleTurn(Battle &b, Action a0, Action a1) {
  if (b.outcome) return;
  b.evCount = 0;
  b.turn++;
  Action act[2] = { a0, a1 };
  for (int s = 0; s < 2; s++) {
    b.side[s].vol.lastDamage = 0;
    const Volatile &v = b.side[s].vol;
    if (v.charging || v.lockTurns || v.bideTurns || v.recharge) act[s].kind = ACT_MOVE;  // forzado
  }

  // huir, cambiar y objetos van antes que los ataques
  for (int s = 0; s < 2; s++) {
    if (act[s].kind == ACT_RUN) {
      if (b.wild) { b.outcome = OUT_FLED; emit(b, EV_EFFECT, s, 0xF3); return; }
      act[s].kind = ACT_MOVE;
    }
  }
  for (int s = 0; s < 2; s++) {
    if (act[s].kind == ACT_SWITCH) battleSwitchIn(b, s, act[s].index);
    else if (act[s].kind == ACT_ITEM) {
      BattleSide &sd = b.side[s];
      if (sd.potions) { sd.potions--; emit(b, EV_ITEM, s); heal(b, s, sd.potionHeal); }
    }
  }

  // orden de los ataques: prioridad y luego velocidad
  int first = 0;
  if (act[0].kind == ACT_MOVE && act[1].kind == ACT_MOVE) {
    const BattleMon &m0 = activeMon(b, 0), &m1 = activeMon(b, 1);
    int8_t p0 = act[0].index < 4 ? MOVE_TBL[m0.moves[act[0].index]].priority : 0;
    int8_t p1 = act[1].index < 4 ? MOVE_TBL[m1.moves[act[1].index]].priority : 0;
    if (b.side[0].vol.charging || b.side[0].vol.lockTurns) p0 = 0;
    if (b.side[1].vol.charging || b.side[1].vol.lockTurns) p1 = 0;
    if (p0 != p1) first = p0 > p1 ? 0 : 1;
    else {
      uint16_t s0 = effectiveSpeed(m0, b.side[0].vol), s1 = effectiveSpeed(m1, b.side[1].vol);
      first = s0 > s1 ? 0 : s1 > s0 ? 1 : b.rng.range(0, 1);
    }
  }
  for (int k = 0; k < 2 && !b.outcome; k++) {
    int s = k == 0 ? first : foeOf(first);
    if (act[s].kind != ACT_MOVE) continue;
    if (!activeMon(b, s).hp || !activeMon(b, foeOf(s)).hp) continue;
    runMoveAction(b, s, act[s].index);
    checkOutcome(b);
  }
  if (b.outcome) return;
  for (int k = 0; k < 2; k++) endOfTurn(b, k == 0 ? first : foeOf(first));
  for (int s = 0; s < 2; s++) if (!activeMon(b, s).hp) restoreSaved(b.side[s]);
  checkOutcome(b);
}

// ---------------------------------------------------------------- IA -------

uint8_t aiTierForLevel(uint8_t level) {
  return level < 20 ? 1 : level < 35 ? 2 : level < 50 ? 3 : 4;
}

// valor de un movimiento en "% de PS del rival" (mas = mejor)
static int scoreMove(Battle &b, int s, uint8_t slot, uint8_t tier) {
  const BattleSide &as = b.side[s], &ds = b.side[foeOf(s)];
  const BattleMon &att = as.party[as.active], &def = ds.party[ds.active];
  uint8_t move = att.moves[slot];
  const MoveInfo &mi = MOVE_TBL[move];
  if (mi.power || isFixedDamage(move) || mi.category == MC_OHKO) {
    uint32_t dmg;
    if (isFixedDamage(move)) dmg = move == MV_SUPER_FANG ? def.hp / 2 : move == MV_SONIC_BOOM ? 20 :
                                   move == MV_DRAGON_RAGE ? 40 : att.level;
    else if (mi.category == MC_OHKO) dmg = (att.spe >= def.spe && typeEff2(mi.type, def.type1, def.type2)) ? def.hp * 30 / 100 : 0;
    else dmg = calcDamage(att, as.vol, def, ds.vol, move, false, 236, nullptr);
    if (move == MV_DREAM_EATER && def.status != STS_SLP) dmg = 0;
    if (isTwoTurn(move) || move == MV_HYPER_BEAM) dmg = dmg * 6 / 10;  // pierde un turno
    if (move == MV_SELF_DESTRUCT || move == MV_EXPLOSION) dmg = att.hp * 3 < att.maxHp ? dmg : dmg / 3;
    uint32_t hits = mi.maxHits ? (mi.minHits == mi.maxHits ? mi.maxHits : 3) : 1;
    dmg *= hits;
    uint32_t acc = mi.accuracy ? mi.accuracy : 100;
    int sc = (int)((dmg > def.hp ? def.hp : dmg) * 100 / def.maxHp * acc / 100);
    if (dmg >= def.hp) sc += tier >= 4 ? 60 + (int)acc / 2 + mi.priority * 20 : 40;  // remata
    return sc;
  }
  if (tier < 3) return 3;  // los niveles bajos casi no usan estados
  switch (mi.ailment) {
    case AIL_SLEEP: return def.status ? 0 : 45;
    case AIL_PARALYSIS: return def.status || !typeEff2(mi.type, def.type1, def.type2) ? 0 : 35;
    case AIL_POISON: return def.status || hasType(def, T_POISON) ? 0 : (move == MV_TOXIC ? 35 : 22);
    case AIL_CONFUSION: return ds.vol.confused ? 0 : 20;
    case AIL_LEECH_SEED: return ds.vol.seeded || hasType(def, T_GRASS) ? 0 : 25;
    default: break;
  }
  if (mi.category == MC_HEAL || move == MV_REST) return att.hp * 2 < att.maxHp ? 45 : 0;
  if (mi.stat1 != ST_NONE) {
    if (mi.selfStat) {
      int8_t st = as.vol.stage[mi.stat1 - ST_ATTACK];
      return (st < 2 && att.hp * 3 > att.maxHp * 2) ? 22 : 0;
    }
    int8_t st = ds.vol.stage[mi.stat1 - ST_ATTACK];
    return st > -2 ? 10 : 0;
  }
  if (move == MV_REFLECT) return as.vol.reflect ? 0 : 18;
  if (move == MV_LIGHT_SCREEN) return as.vol.lightScreen ? 0 : 18;
  if (move == MV_SUBSTITUTE) return as.vol.substituteHp || att.hp * 2 < att.maxHp ? 0 : 15;
  return 5;
}

// lo bien que le va al Pokemon `m` contra el rival activo (dano que hace - dano que recibe)
static int matchup(Battle &b, int s, const BattleMon &m) {
  const BattleMon &foe = activeMon(b, foeOf(s));
  int best = 0, worst = 0;
  for (int i = 0; i < 4; i++) {
    if (m.moves[i] && MOVE_TBL[m.moves[i]].power) {
      int d = estimateDamage(m, foe, m.moves[i]) * 100 / foe.maxHp;
      if (d > best) best = d;
    }
    if (foe.moves[i] && MOVE_TBL[foe.moves[i]].power) {
      int d = estimateDamage(foe, m, foe.moves[i]) * 100 / m.maxHp;
      if (d > worst) worst = d;
    }
  }
  return best - worst;
}

uint8_t battleAiPickSwitch(Battle &b, int s) {
  BattleSide &sd = b.side[s];
  int bestIdx = -1, bestScore = -10000;
  for (int i = 0; i < sd.count; i++) {
    if (i == sd.active || !sd.party[i].hp) continue;
    int sc = sd.aiTier >= 2 ? matchup(b, s, sd.party[i]) : -i;
    if (sc > bestScore) { bestScore = sc; bestIdx = i; }
  }
  return bestIdx < 0 ? sd.active : (uint8_t)bestIdx;
}

Action battleAiChoose(Battle &b, int s) {
  BattleSide &sd = b.side[s];
  BattleMon &m = sd.party[sd.active];
  uint8_t tier = sd.aiTier;
  Action a;
  a.kind = ACT_MOVE;
  uint8_t usable[4], n = 0;
  for (int i = 0; i < 4; i++) if (canUseMove(b, s, i)) usable[n++] = i;
  if (!n) { a.index = 4; return a; }  // Forcejeo
  if (tier <= 1) { a.index = usable[b.rng.range(0, n - 1)]; return a; }

  if (tier >= 3) {
    // curarse si esta en las ultimas y la cura sirve de algo
    if (sd.potions && m.hp * 4 < m.maxHp && sd.potionHeal >= m.maxHp / 4) {
      a.kind = ACT_ITEM;
      return a;
    }
    // cambiar si el emparejamiento es muy malo y hay alguien mucho mejor
    int cur = matchup(b, s, m);
    if (cur < -40) {
      uint8_t idx = battleAiPickSwitch(b, s);
      if (idx != sd.active && matchup(b, s, sd.party[idx]) > cur + 50 &&
          (tier >= 4 || b.rng.chance(50))) {
        a.kind = ACT_SWITCH;
        a.index = idx;
        return a;
      }
    }
  }

  int bestScore = -1, best = usable[0];
  for (int k = 0; k < n; k++) {
    int sc = scoreMove(b, s, usable[k], tier);
    if (tier == 2) sc += b.rng.range(0, 15);  // algo de ruido: no siempre lo optimo
    if (tier == 3) sc += b.rng.range(0, 6);
    if (sc > bestScore) { bestScore = sc; best = usable[k]; }
  }
  if (tier == 2 && b.rng.chance(20)) best = usable[b.rng.range(0, n - 1)];
  a.index = (uint8_t)best;
  return a;
}

// ---------------------------------------------------------------- captura --

uint16_t catchChancePermille(const BattleMon &wild, uint8_t ball, uint8_t throwX10) {
  if (ball == 0) return 1000;  // Master Ball
  uint32_t M3 = 3u * wild.maxHp;
  uint32_t statusX10 = (wild.status == STS_SLP || wild.status == STS_FRZ) ? 20 : wild.status ? 15 : 10;
  // a = (3M - 2H) * captura * bola / 3M * estado * lanzamiento; probabilidad = a / 255
  uint64_t a = (uint64_t)(M3 - 2u * wild.hp) * BSPECIES[wild.species].captureRate * ball * statusX10 * throwX10;
  uint64_t den = (uint64_t)M3 * 10 * 10 * 10;
  uint64_t pm = a * 1000 / den / 255;
  return pm > 1000 ? 1000 : (uint16_t)pm;
}
