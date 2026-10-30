#include "gyms.h"

const GymInfo GYMS[NUM_GYMS] = {
  // lider       tipo        km  +nv  eq  as   as alto  resto del equipo
  { "Brock",     T_ROCK,      0, 0, 2,  95, 142, { 74, 111, 138, 140, 27, 104, 0, 0 } },
  { "Misty",     T_WATER,     3, 1, 3, 120, 131, { 54, 116, 118, 90, 60, 72, 86, 0 } },
  { "Lt. Surge", T_ELECTRIC,  8, 2, 3,  25, 133, { 100, 81, 125, 0, 0, 0, 0, 0 } },
  { "Erika",     T_GRASS,    15, 3, 4,  43, 102, { 69, 114, 46, 1, 0, 0, 0, 0 } },
  { "Koga",      T_POISON,   25, 4, 4, 109,  32, { 88, 23, 41, 48, 72, 29, 0, 0 } },
  { "Sabrina",   T_PSYCHIC,  35, 5, 5,  63,  63, { 96, 122, 79, 124, 102, 0, 0, 0 } },
  { "Blaine",    T_FIRE,     50, 6, 5,  58,   4, { 77, 37, 126, 133, 0, 0, 0, 0 } },
  { "Giovanni",  T_GROUND,   65, 8, 6, 111, 111, { 32, 29, 50, 27, 104, 0, 0, 0 } },
};

uint8_t baseLevel(const BattleMon *party, uint8_t count) {
  if (!count) return 1;
  uint16_t hi = 0, sum = 0;
  for (int i = 0; i < count; i++) {
    if (party[i].level > hi) hi = party[i].level;
    sum += party[i].level;
  }
  // 0.6 * alto + 0.4 * media, redondeado, en enteros: (6*alto*n + 4*suma + 5n) / 10n
  uint32_t n = count;
  return (uint8_t)((6u * hi * n + 4u * sum + 5u * n) / (10u * n));
}

uint8_t gymTmTier(uint8_t aceLevel) { return aceLevel < 25 ? 0 : aceLevel < 45 ? 1 : 2; }

uint16_t potionForLevel(uint8_t level) {
  return level < 20 ? 20 : level < 35 ? 50 : level < 50 ? 200 : 999;
}

// movimientos que la eleccion automatica no coge: se pierden turnos o se debilitan
static bool skipForSmart(uint8_t move) {
  const MoveInfo &mi = MOVE_TBL[move];
  if (mi.category == MC_OHKO) return true;
  switch (move) {
    case 13: case 19: case 76: case 91: case 130: case 143:  // dos turnos
    case 120: case 153:                                      // autodestruccion
    case 117: case 68: case 118: case 119: case 102: case 144: case 150: case 100:
    case 6:                                                  // Dia de Pago
      return true;
    default:
      return false;
  }
}

static int statusValue(uint8_t move) {
  const MoveInfo &mi = MOVE_TBL[move];
  if (mi.power) return 0;
  if (mi.ailment == AIL_SLEEP) return 60 + mi.accuracy / 4;
  if (mi.ailment == AIL_PARALYSIS) return 50 + mi.accuracy / 4;
  if (move == 92) return 45;                                  // Toxico
  if (mi.category == MC_HEAL) return 40;
  if (mi.stat1 != ST_NONE && mi.selfStat && mi.delta1 >= 2) return 35;  // Danza Espada, Amnesia...
  if (mi.ailment == AIL_CONFUSION) return 20 + mi.accuracy / 10;
  if (move == 73) return 30;                                  // Drenadoras
  return 0;
}

void pickMovesSmart(BattleMon &m, const BattleMon *foes, uint8_t nFoes, bool machines) {
  const BattleSpecies &s = BSPECIES[m.species];
  uint8_t cand[64];
  uint8_t n = 0;
  auto add = [&](uint8_t mv) {
    if (!mv || skipForSmart(mv)) return;
    for (int i = 0; i < n; i++) if (cand[i] == mv) return;
    if (n < sizeof(cand)) cand[n++] = mv;
  };
  for (int i = 0; i < s.learnCount; i++) {
    const LearnEntry &e = LEARN_TBL[s.learnStart + i];
    if (e.level <= m.level) add(e.move);
  }
  if (machines)
    for (int i = 0; i < NUM_MACHINES; i++)
      if ((s.machines >> i) & 1ULL) add(MACHINE_MOVE[i]);

  // valor de dano: poder * precision * STAB * eficacia media contra los rivales
  int value[64];
  for (int i = 0; i < n; i++) {
    const MoveInfo &mi = MOVE_TBL[cand[i]];
    if (!mi.power) { value[i] = 0; continue; }
    uint32_t v = (uint32_t)mi.power * (mi.accuracy ? mi.accuracy : 100);
    if (m.type1 == mi.type || m.type2 == mi.type) v = v * 3 / 2;
    if (mi.maxHits) v *= (mi.minHits == mi.maxHits) ? mi.maxHits : 3;
    if (cand[i] == 63) v = v * 6 / 10;  // Hiperrayo recarga
    // el ataque debe ir con la stat buena del Pokemon
    bool phys = mi.type < T_FIRE;
    uint16_t a = phys ? s.atk : s.spc;
    v = v * a / 100;
    uint32_t eff = 0;
    for (int f = 0; f < nFoes; f++) eff += typeEff2(mi.type, foes[f].type1, foes[f].type2);
    v = nFoes ? v * eff / (2u * nFoes) : v;
    value[i] = (int)(v / 100);
  }

  uint8_t chosen[4] = {0, 0, 0, 0};
  uint8_t types[4] = {T_NONE, T_NONE, T_NONE, T_NONE};
  int k = 0;
  // hasta 3 de dano, de tipos distintos
  while (k < 3) {
    int best = -1;
    for (int i = 0; i < n; i++) {
      if (value[i] <= 0) continue;
      bool clash = false;
      for (int j = 0; j < k; j++) clash |= chosen[j] == cand[i] || types[j] == MOVE_TBL[cand[i]].type;
      if (!clash && (best < 0 || value[i] > value[best])) best = i;
    }
    if (best < 0) break;
    chosen[k] = cand[best];
    types[k] = MOVE_TBL[cand[best]].type;
    k++;
  }
  // un movimiento de estado si merece la pena
  int bestSt = -1, bestStV = 0;
  for (int i = 0; i < n; i++) {
    int v = statusValue(cand[i]);
    if (v > bestStV) { bestStV = v; bestSt = i; }
  }
  if (bestSt >= 0 && k < 4) chosen[k++] = cand[bestSt];
  // rellena con el mejor dano restante, aunque repita tipo
  while (k < 4) {
    int best = -1;
    for (int i = 0; i < n; i++) {
      bool dup = false;
      for (int j = 0; j < k; j++) dup |= chosen[j] == cand[i];
      if (!dup && value[i] > 0 && (best < 0 || value[i] > value[best])) best = i;
    }
    if (best < 0) break;
    chosen[k++] = cand[best];
  }
  if (!k) { monDefaultMoves(m); return; }
  for (int i = 0; i < 4; i++) monSetMove(m, i, i < k ? chosen[i] : 0);
}

void buildGymTeam(BattleSide &leader, uint8_t gym, const BattleSide &challenger, Rng &rng) {
  const GymInfo &g = GYMS[gym];
  uint8_t bl = baseLevel(challenger.party, challenger.count);
  int top = bl + g.levelOffset;
  if (top > 100) top = 100;
  uint8_t aceLv = (uint8_t)(top < 2 ? 2 : top);
  uint8_t tier = aiTierForLevel(aceLv) + 1;
  if (tier > 4) tier = 4;

  // el resto del equipo: al azar del grupo, sin repetir
  uint8_t poolN = 0;
  while (poolN < 8 && g.pool[poolN]) poolN++;
  uint8_t order[8];
  for (int i = 0; i < poolN; i++) order[i] = g.pool[i];
  for (int i = poolN - 1; i > 0; i--) {
    int j = rng.range(0, i);
    uint8_t t = order[i]; order[i] = order[j]; order[j] = t;
  }

  leader = BattleSide();
  leader.aiTier = tier;
  leader.count = g.teamSize;
  for (int i = 0; i < g.teamSize; i++) {
    bool isAce = i == g.teamSize - 1;
    uint8_t lv = isAce ? aceLv : (uint8_t)(aceLv > 3 ? aceLv - rng.range(1, 3) : aceLv);
    uint8_t base = isAce ? (aceLv >= 35 ? g.aceHigh : g.ace) : order[i % poolN];
    uint8_t sp = evolveForLevel(base, lv, g.type);
    uint8_t genes[5];
    for (int k = 0; k < 5; k++) genes[k] = (uint8_t)rng.range(105, 115);
    monInit(leader.party[i], sp, lv, genes);
    if (tier >= 3) pickMovesSmart(leader.party[i], challenger.party, challenger.count, true);
  }
  leader.active = 0;
  leader.potions = tier >= 4 ? 2 : tier >= 3 ? 1 : 0;
  leader.potionHeal = potionForLevel(aceLv);
}
