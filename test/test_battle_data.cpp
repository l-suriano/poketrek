// Integridad de las tablas de batalla (battle_data.h, generada por tools/gen_battle.py).
#include "framework.h"
#include "shim/Arduino.h"
#include "../battle_data.h"
#include <string.h>

TEST(battle_data, tamanos_de_las_tablas) {
  CHECK_EQ((int)(sizeof(MOVE_TBL) / sizeof(MOVE_TBL[0])), NUM_MOVES + 1);
  CHECK_EQ((int)(sizeof(BSPECIES) / sizeof(BSPECIES[0])), 152);
  CHECK_EQ((int)(sizeof(MACHINE_MOVE) / sizeof(MACHINE_MOVE[0])), 55);
  CHECK_EQ(NUM_EVOS, 72);
}

TEST(battle_data, fantasma_pega_x2_a_psiquico) {
  // el bug de gen 1 (x0) esta corregido en PokeTrek
  CHECK_EQ((int)TYPE_EFF[T_GHOST][T_PSYCHIC], 4);
  CHECK_EQ((int)TYPE_EFF[T_GHOST][T_NORMAL], 0);
  CHECK_EQ((int)TYPE_EFF[T_ELECTRIC][T_GROUND], 0);
}

TEST(battle_data, eficacia_contra_dos_tipos) {
  // Roca contra Charizard (fuego/volador) = x4
  CHECK_EQ((int)typeEff2(T_ROCK, BSPECIES[6].type1, BSPECIES[6].type2), 8);
  // Electrico contra Pikachu (un solo tipo) = x0.5
  CHECK_EQ((int)typeEff2(T_ELECTRIC, BSPECIES[25].type1, BSPECIES[25].type2), 1);
}

TEST(battle_data, cada_especie_tiene_tipos_y_aprendizaje_validos) {
  for (int d = 1; d <= 151; d++) {
    const BattleSpecies &s = BSPECIES[d];
    CHECK_MSG(s.type1 < T_COUNT, "type1 fuera de rango");
    CHECK_MSG(s.type2 == T_NONE || s.type2 < T_COUNT, "type2 fuera de rango");
    CHECK_MSG(s.learnCount > 0, "especie sin movimientos por nivel");
    CHECK_MSG(s.captureRate >= 3, "capture rate imposible");
    CHECK_MSG(s.flee >= 5 && s.flee <= 25, "huida fuera de 5..25 %");
    for (int i = 0; i < s.learnCount; i++) {
      const LearnEntry &e = LEARN_TBL[s.learnStart + i];
      CHECK_MSG(e.move >= 1 && e.move <= NUM_MOVES, "movimiento fuera de rango");
      CHECK_MSG(e.level >= 1 && e.level <= 100, "nivel fuera de rango");
    }
  }
}

TEST(battle_data, pikachu_puede_aprender_rayo) {
  // MT24 = Thunderbolt (id 85)
  CHECK_EQ((int)MACHINE_MOVE[23], 85);
  CHECK((BSPECIES[25].machines >> 23) & 1ULL);
}

TEST(battle_data, nombres_por_idioma) {
  gLang = LANG_ES;
  CHECK(strcmp(moveName(85), "Rayo") == 0);
  CHECK(strcmp(typeName(T_FIRE), "Fuego") == 0);
  gLang = LANG_EN;
  CHECK(strcmp(moveName(85), "Thunderbolt") == 0);
  CHECK(strcmp(typeName(T_FIRE), "Fire") == 0);
}
