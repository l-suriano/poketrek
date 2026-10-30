// Simulador de PokeTrek: miles de peleas contra cada gimnasio para afinar la
// dificultad (reglas: "Tuning, data & licensing"). Corre en el PC con el mismo
// motor que la placa.
//
//   make            compila y ejecuta con los valores por defecto
//   ./sim 1000      1000 batallas por casilla (mas lento, mas preciso)
//
// Dos perfiles de jugador:
//   casual    6 Pokemon al azar, movimientos por defecto, IA nivel 2, 2 pociones
//   preparado al menos 3 con ventaja de tipo contra el lider, movimientos con
//             cobertura (incluye MT), IA nivel 4, 5 pociones
#include "../battle.h"
#include "../gyms.h"
#include <stdio.h>
#include <stdlib.h>

static const uint8_t LEVELS[] = { 12, 20, 30, 40, 50, 60 };
#define NUM_LEVELS (sizeof(LEVELS) / sizeof(LEVELS[0]))

static bool isLegendary(uint8_t sp) { return sp == 144 || sp == 145 || sp == 146 || sp == 150 || sp == 151; }

static bool isBaseForm(uint8_t sp) {
  for (int i = 0; i < NUM_EVOS; i++) if (EVO_TBL[i].to == sp) return false;
  return true;
}

static uint8_t randomBase(Rng &rng) {
  for (;;) {
    uint8_t sp = (uint8_t)rng.range(1, 151);
    if (!isLegendary(sp) && isBaseForm(sp)) return sp;
  }
}

// algun tipo propio del Pokemon pega x2 al tipo del gimnasio
static bool hasAdvantage(uint8_t sp, uint8_t gymType) {
  const BattleSpecies &s = BSPECIES[sp];
  if (TYPE_EFF[s.type1][gymType] == 4) return true;
  return s.type2 != T_NONE && TYPE_EFF[s.type2][gymType] == 4;
}

static void makeMon(BattleMon &m, uint8_t base, uint8_t level, Rng &rng) {
  uint8_t genes[5];
  uint16_t sexp[5];
  for (int k = 0; k < 5; k++) {
    genes[k] = (uint8_t)rng.range(90, 110);
    uint32_t e = (uint32_t)level * level * 8;  // un equipo entrenado: Stat Exp de gen 1
    sexp[k] = (uint16_t)(e > 65535 ? 65535 : e);
  }
  monInit(m, evolveForLevel(base, level), level, genes, sexp);
}

static void buildPlayer(BattleSide &p, uint8_t level, bool prepared, uint8_t gym, Rng &rng) {
  p = BattleSide();
  p.count = 6;
  uint8_t used[6] = {0};
  for (int i = 0; i < 6; i++) {
    uint8_t sp;
    bool dup;
    do {
      sp = randomBase(rng);
      // el preparado lleva al menos 3 con ventaja de tipo (miramos la forma evolucionada)
      if (prepared && i < 3 && !hasAdvantage(evolveForLevel(sp, level), GYMS[gym].type)) { dup = true; continue; }
      dup = false;
      for (int j = 0; j < i; j++) dup |= used[j] == sp;
    } while (dup);
    used[i] = sp;
    makeMon(p.party[i], sp, level, rng);
  }
  if (prepared) {
    // cobertura pensada contra lo que suele sacar el lider: su tipo
    BattleMon dummy;
    monInit(dummy, GYMS[gym].ace, level);
    dummy.type1 = GYMS[gym].type;
    dummy.type2 = T_NONE;
    for (int i = 0; i < 6; i++) pickMovesSmart(p.party[i], &dummy, 1, true);
  }
  p.aiTier = prepared ? 4 : 2;
  p.potions = prepared ? 5 : 2;
  p.potionHeal = potionForLevel(level);
}

// devuelve true si gana el jugador
static bool fight(uint8_t gym, uint8_t level, bool prepared, uint32_t seed) {
  Rng rng;
  rng.seed(seed * 2654435761u + gym * 97 + level);
  Battle b;
  b = Battle();
  buildPlayer(b.side[0], level, prepared, gym, rng);
  buildGymTeam(b.side[1], gym, b.side[0], rng);
  battleInit(b, rng.next(), false);
  for (int turn = 0; turn < 1500 && !b.outcome; turn++) {
    for (int s = 0; s < 2; s++)
      if (battleNeedsSwitch(b, s)) battleSwitchIn(b, s, battleAiPickSwitch(b, s));
    battleTurn(b, battleAiChoose(b, 0), battleAiChoose(b, 1));
  }
  return b.outcome == OUT_WIN0;
}

int main(int argc, char **argv) {
  int n = argc > 1 ? atoi(argv[1]) : 300;
  if (n < 1) n = 1;
  printf("PokeTrek - %% de victorias del jugador contra cada gimnasio (%d batallas por casilla)\n", n);
  printf("Jugador con los 6 Pokemon al nivel indicado (Base Level = ese nivel).\n");
  printf("Objetivo de las reglas: 35-50 %% al primer intento con un equipo bien armado.\n\n");
  for (int prof = 1; prof >= 0; prof--) {
    bool prepared = prof == 1;
    printf("== jugador %s ==\n", prepared ? "PREPARADO (cobertura, IA 4, 5 pociones)" : "CASUAL (al azar, IA 2, 2 pociones)");
    printf("%-10s", "Lider");
    for (unsigned l = 0; l < NUM_LEVELS; l++) printf("  Nv%-4d", LEVELS[l]);
    printf("\n");
    for (int g = 0; g < NUM_GYMS; g++) {
      printf("%-10s", GYMS[g].leader);
      for (unsigned l = 0; l < NUM_LEVELS; l++) {
        int wins = 0;
        for (int i = 0; i < n; i++) wins += fight((uint8_t)g, LEVELS[l], prepared, (uint32_t)i + 1);
        printf("  %4.0f%% ", 100.0 * wins / n);
      }
      printf("\n");
      fflush(stdout);
    }
    printf("\n");
  }
  return 0;
}
