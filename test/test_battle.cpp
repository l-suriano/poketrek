// Motor de batallas (battle.cpp) y gimnasios (gyms.cpp).
#include "framework.h"
#include "shim/Arduino.h"
#include "../battle.h"
#include "../gyms.h"

// ids de PokeAPI que usan los tests
enum { PIKACHU = 25, RAICHU = 26, CHARMANDER = 4, CHARIZARD = 6, BULBASAUR = 1, SQUIRTLE = 7,
       GEODUDE = 74, GASTLY = 92, ABRA = 63, KADABRA = 64, ALAKAZAM = 65, EEVEE = 133, FLAREON = 136,
       PIDGEY = 16, DRATINI = 147, SNORLAX = 143 };
enum { M_THUNDER_SHOCK = 84, M_THUNDERBOLT = 85, M_EMBER = 52, M_TACKLE = 33, M_LICK = 122,
       M_QUICK_ATTACK = 98, M_SWIFT = 129, M_AGILITY = 97, M_THUNDER = 87, M_HYPER_BEAM = 63,
       M_FLY = 19, M_TOXIC = 92, M_SPLASH = 150 };

static void oneVsOne(Battle &b, uint8_t sp0, uint8_t lv0, uint8_t sp1, uint8_t lv1, uint32_t seed = 1) {
  b = Battle();
  b.side[0].count = 1;
  b.side[1].count = 1;
  monInit(b.side[0].party[0], sp0, lv0);
  monInit(b.side[1].party[0], sp1, lv1);
  battleInit(b, seed, false);
}

static int countEv(const Battle &b, uint8_t kind, int side = -1) {
  int n = 0;
  for (int i = 0; i < b.evCount; i++)
    if (b.ev[i].kind == kind && (side < 0 || b.ev[i].side == side)) n++;
  return n;
}

static Action mv(uint8_t slot) { Action a; a.kind = ACT_MOVE; a.index = slot; return a; }

TEST(battle, stats_de_gen_1_con_dv_medio) {
  BattleMon m;
  monInit(m, PIKACHU, 50);
  // HP: (35+8)*2*50/100 + 50 + 10 = 103; Ataque: (55+8)*2*50/100 + 5 = 68
  CHECK_EQ((int)m.maxHp, 103);
  CHECK_EQ((int)m.atk, 68);
  CHECK_EQ((int)m.hp, (int)m.maxHp);
}

TEST(battle, los_genes_mueven_las_stats) {
  BattleMon lo, hi;
  uint8_t g90[5] = {90, 90, 90, 90, 90}, g110[5] = {110, 110, 110, 110, 110};
  monInit(lo, SNORLAX, 40, g90);
  monInit(hi, SNORLAX, 40, g110);
  CHECK(hi.maxHp > lo.maxHp);
  CHECK(hi.atk > lo.atk);
}

TEST(battle, movimientos_por_defecto_los_4_ultimos) {
  BattleMon m;
  monInit(m, PIKACHU, 50);
  CHECK_EQ((int)m.moves[0], M_QUICK_ATTACK);
  CHECK_EQ((int)m.moves[1], M_SWIFT);
  CHECK_EQ((int)m.moves[2], M_AGILITY);
  CHECK_EQ((int)m.moves[3], M_THUNDER);
  CHECK_EQ((int)m.pp[3], 10);
}

TEST(battle, evolucion_segun_nivel) {
  CHECK_EQ((int)evolveForLevel(PIKACHU, 29), PIKACHU);
  CHECK_EQ((int)evolveForLevel(PIKACHU, 30), RAICHU);   // piedra desde el 30
  CHECK_EQ((int)evolveForLevel(CHARMANDER, 36), CHARIZARD);
  CHECK_EQ((int)evolveForLevel(ABRA, 34), KADABRA);
  CHECK_EQ((int)evolveForLevel(ABRA, 35), ALAKAZAM);    // intercambio desde el 35
  CHECK_EQ((int)evolveForLevel(EEVEE, 40, T_FIRE), FLAREON);
}

TEST(battle, eficacia_en_el_dano_estimado) {
  BattleMon ch, bu, sq;
  monInit(ch, CHARMANDER, 20);
  monInit(bu, BULBASAUR, 20);
  monInit(sq, SQUIRTLE, 20);
  CHECK(estimateDamage(ch, bu, M_EMBER) > estimateDamage(ch, sq, M_EMBER) * 3);
}

TEST(battle, normal_no_afecta_a_fantasma_y_fantasma_si_a_psiquico) {
  BattleMon a, g, abra;
  monInit(a, SNORLAX, 30);
  monInit(g, GASTLY, 30);
  monInit(abra, ABRA, 30);
  CHECK_EQ((int)estimateDamage(a, g, M_TACKLE), 0);
  CHECK(estimateDamage(g, abra, M_LICK) > 0);
}

TEST(battle, electrico_no_afecta_a_tierra_en_combate) {
  Battle b;
  oneVsOne(b, PIKACHU, 10, GEODUDE, 10);
  monSetMove(b.side[0].party[0], 0, M_THUNDER_SHOCK);
  monSetMove(b.side[1].party[0], 0, M_SPLASH);
  uint16_t hp = b.side[1].party[0].hp;
  battleTurn(b, mv(0), mv(0));
  CHECK_EQ((int)b.side[1].party[0].hp, (int)hp);
}

TEST(battle, el_mas_rapido_ataca_primero) {
  Battle b;
  oneVsOne(b, PIKACHU, 30, GEODUDE, 30);  // Pikachu base 90 de velocidad, Geodude 20
  battleTurn(b, mv(0), mv(0));
  int firstMove = -1;
  for (int i = 0; i < b.evCount && firstMove < 0; i++) if (b.ev[i].kind == EV_MOVE) firstMove = b.ev[i].side;
  CHECK_EQ(firstMove, 0);
}

TEST(battle, dormido_no_ataca) {
  Battle b;
  oneVsOne(b, PIKACHU, 30, GEODUDE, 30);
  BattleMon &p = b.side[0].party[0];
  p.status = STS_SLP;
  p.sleepTurns = 3;
  battleTurn(b, mv(0), mv(0));
  CHECK_EQ(countEv(b, EV_MOVE, 0), 0);
  CHECK_EQ(countEv(b, EV_ASLEEP, 0), 1);
}

TEST(battle, toxico_hace_cada_vez_mas_dano) {
  Battle b;
  oneVsOne(b, SNORLAX, 50, SNORLAX, 50);
  monSetMove(b.side[0].party[0], 0, M_SPLASH);
  monSetMove(b.side[1].party[0], 0, M_SPLASH);
  BattleMon &t = b.side[1].party[0];
  t.status = STS_TOX;
  t.toxicN = 1;
  uint16_t h0 = t.hp;
  battleTurn(b, mv(0), mv(0));
  uint16_t d1 = h0 - t.hp, h1 = t.hp;
  battleTurn(b, mv(0), mv(0));
  uint16_t d2 = h1 - t.hp;
  CHECK(d2 > d1);
}

TEST(battle, hiperrayo_recarga_si_no_debilita) {
  Battle b;
  oneVsOne(b, SNORLAX, 30, SNORLAX, 60);
  monSetMove(b.side[0].party[0], 0, M_HYPER_BEAM);
  monSetMove(b.side[1].party[0], 0, M_SPLASH);
  b.side[0].party[0].moves[1] = 0;
  battleTurn(b, mv(0), mv(0));
  if (b.side[1].party[0].hp && countEv(b, EV_MISS, 0) == 0) {
    CHECK(b.side[0].vol.recharge);
    battleTurn(b, mv(0), mv(0));
    CHECK_EQ(countEv(b, EV_RECHARGE, 0), 1);
  }
}

TEST(battle, vuelo_es_invulnerable_en_la_carga) {
  Battle b;
  oneVsOne(b, PIDGEY, 30, SNORLAX, 30);
  monSetMove(b.side[0].party[0], 0, M_FLY);
  monSetMove(b.side[1].party[0], 0, M_TACKLE);
  b.side[1].party[0].moves[1] = b.side[1].party[0].moves[2] = b.side[1].party[0].moves[3] = 0;
  uint16_t hp = b.side[0].party[0].hp;
  battleTurn(b, mv(0), mv(0));
  CHECK_EQ(countEv(b, EV_CHARGE, 0), 1);
  if (b.side[0].vol.invulnerable || countEv(b, EV_MISS, 1)) CHECK_EQ((int)b.side[0].party[0].hp, (int)hp);
}

TEST(battle, confianza_baja_desobedece_y_alta_nunca) {
  int disobey0 = 0, disobey100 = 0;
  for (uint32_t seed = 1; seed <= 200; seed++) {
    Battle b;
    oneVsOne(b, SNORLAX, 50, SNORLAX, 50, seed);
    b.side[0].party[0].trust = 0;
    battleTurn(b, mv(0), mv(0));
    disobey0 += countEv(b, EV_DISOBEY, 0);
    oneVsOne(b, SNORLAX, 50, SNORLAX, 50, seed);
    battleTurn(b, mv(0), mv(0));
    disobey100 += countEv(b, EV_DISOBEY, 0);
  }
  CHECK_RANGE(disobey0, 70, 130);  // 50 % esperado
  CHECK_EQ(disobey100, 0);
}

TEST(battle, captura_segun_las_reglas) {
  BattleMon p, d;
  monInit(p, PIDGEY, 10);
  CHECK_EQ((int)catchChancePermille(p, 10, 10), 333);  // PS llenos, Poke Ball, tiro normal: 33 %
  monInit(d, DRATINI, 10);
  d.hp = 1;
  d.status = STS_SLP;
  CHECK_EQ((int)catchChancePermille(d, 20, 16), 1000);  // 1 PS, dormido, Ultra Ball, tiro Great
  CHECK_EQ((int)catchChancePermille(d, 0, 10), 1000);   // Master Ball
}

TEST(battle, base_level_pesa_el_mas_alto) {
  BattleMon party[6];
  monInit(party[0], SNORLAX, 50);
  for (int i = 1; i < 6; i++) monInit(party[i], PIDGEY, 10);
  CHECK_EQ((int)baseLevel(party, 6), 37);  // 0.6*50 + 0.4*16.7 = 36.7
  CHECK_EQ((int)baseLevel(party, 1), 50);
}

TEST(battle, equipo_de_gimnasio_escala_con_el_jugador) {
  BattleSide me;
  me.count = 6;
  for (int i = 0; i < 6; i++) monInit(me.party[i], SNORLAX, 40);
  Rng rng;
  BattleSide brock, gio;
  buildGymTeam(brock, 0, me, rng);
  buildGymTeam(gio, 7, me, rng);
  CHECK_EQ((int)brock.count, 2);
  CHECK_EQ((int)brock.party[1].level, 40);   // as = BL + 0
  CHECK_EQ((int)gio.count, 6);
  CHECK_EQ((int)gio.party[5].level, 48);     // as = BL + 8
  CHECK_EQ((int)gio.aiTier, 4);
  for (int i = 0; i < 6; i++) CHECK(gio.party[i].moves[0] != 0);
}

TEST(battle, batallas_completas_terminan_y_no_rompen_ps) {
  Rng pick;
  for (uint32_t seed = 1; seed <= 150; seed++) {
    Battle b;
    b = Battle();
    for (int s = 0; s < 2; s++) {
      b.side[s].count = 3;
      b.side[s].aiTier = (uint8_t)(1 + seed % 4);
      for (int i = 0; i < 3; i++) {
        uint8_t sp = (uint8_t)pick.range(1, 151);
        monInit(b.side[s].party[i], sp, (uint8_t)pick.range(5, 70));
      }
    }
    battleInit(b, seed, false);
    int turns = 0;
    while (!b.outcome && turns < 2000) {
      for (int s = 0; s < 2; s++)
        if (battleNeedsSwitch(b, s)) battleSwitchIn(b, s, battleAiPickSwitch(b, s));
      battleTurn(b, battleAiChoose(b, 0), battleAiChoose(b, 1));
      for (int s = 0; s < 2; s++)
        for (int i = 0; i < b.side[s].count; i++)
          CHECK_MSG(b.side[s].party[i].hp <= b.side[s].party[i].maxHp, "PS por encima del maximo");
      turns++;
    }
    CHECK_MSG(b.outcome != OUT_NONE, "la batalla no termino en 2000 turnos");
  }
}
