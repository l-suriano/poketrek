// Nucleo del entrenador (trainer.cpp): experiencia, evoluciones, confianza,
// captura, equipo, objetos, dinero y guardado.
#include "framework.h"
#include "shim/Arduino.h"
#include "../trainer.h"
#include <string.h>
#include <vector>

enum { BULBASAUR = 1, CHARMANDER = 4, CHARMELEON = 5, PIDGEY = 16, PIKACHU = 25, RAICHU = 26,
       EEVEE = 133, VAPOREON = 134, KADABRA = 64, ALAKAZAM = 65, SNORLAX = 143 };
enum { M_THUNDER_WAVE = 86, M_THUNDERBOLT = 85 };

static Trainer *freshTrainer() {
  static Trainer t;
  t = Trainer();
  return &t;
}

TEST(trainer, curvas_de_experiencia_de_gen_1) {
  CHECK_EQ((int)expForLevel(G_MEDIUM, 100), 1000000);
  CHECK_EQ((int)expForLevel(G_FAST, 100), 800000);
  CHECK_EQ((int)expForLevel(G_SLOW, 100), 1250000);
  CHECK_EQ((int)expForLevel(G_MEDIUM_SLOW, 100), 1059860);
  CHECK_EQ((int)expForLevel(G_MEDIUM_SLOW, 2), 9);
  CHECK_EQ((int)expForLevel(G_MEDIUM, 1), 0);
}

TEST(trainer, xp_de_combate_con_bonos) {
  ExpBonus none = {false, false, false, false}, tr = {true, false, false, false};
  uint32_t wild = battleExp(PIDGEY, 10, none);
  CHECK_EQ((int)wild, (int)(BSPECIES[PIDGEY].baseExp * 10 / 7));
  CHECK_EQ((int)battleExp(PIDGEY, 10, tr), (int)(wild * 3 / 2));
  ExpBonus leader = {true, true, false, false};
  CHECK_EQ((int)battleExp(PIDGEY, 10, leader), (int)(wild * 3 / 2 * 2));
}

TEST(trainer, subir_de_nivel_aprende_movimientos_y_cura_lo_ganado) {
  Rng rng;
  OwnedMon m;
  ownedInit(m, PIKACHU, 5, rng);
  uint16_t hp5 = m.hp;
  LevelUp up = gainExp(m, expForLevel(G_MEDIUM, 10) - m.exp);
  CHECK_EQ((int)m.level, 10);
  CHECK_EQ((int)up.levels, 5);
  bool learnsWave = false;
  for (int i = 0; i < up.newMoveCount; i++) learnsWave |= up.newMoves[i] == M_THUNDER_WAVE;
  CHECK(learnsWave);  // Pikachu aprende Onda Trueno al 9
  CHECK(m.hp > hp5);
  CHECK_EQ((int)m.hp, (int)ownedMaxHp(m));
}

TEST(trainer, evolucion_por_nivel_piedra_e_intercambio) {
  Rng rng;
  OwnedMon m;
  ownedInit(m, CHARMANDER, 15, rng);
  LevelUp up = gainExp(m, expForLevel(G_MEDIUM_SLOW, 16) - m.exp);
  CHECK_EQ((int)up.evolveTo, CHARMELEON);
  CHECK_EQ((int)stoneEvolution(PIKACHU, IT_THUNDER_STONE), RAICHU);
  CHECK_EQ((int)stoneEvolution(EEVEE, IT_WATER_STONE), VAPOREON);
  CHECK_EQ((int)stoneEvolution(PIKACHU, IT_FIRE_STONE), 0);
  CHECK_EQ((int)tradeEvolution(KADABRA), ALAKAZAM);
  uint16_t before = m.hp;
  evolve(m, CHARMELEON);
  CHECK_EQ((int)m.species, CHARMELEON);
  CHECK(m.hp > before);
}

TEST(trainer, aprender_movimiento_y_mt) {
  Rng rng;
  OwnedMon m;
  ownedInit(m, PIKACHU, 50, rng);  // ya tiene 4 movimientos
  CHECK(!learnMove(m, M_THUNDERBOLT));          // sin hueco y sin decir cual sustituir
  CHECK(learnMove(m, M_THUNDERBOLT, 0));
  CHECK_EQ((int)m.moves[0], M_THUNDERBOLT);
  CHECK_EQ((int)m.pp[0], 15);
  CHECK(!learnMove(m, M_THUNDERBOLT, 1));       // ya lo sabe
  CHECK(canLearnMachine(PIKACHU, 23));          // MT24 Rayo
}

TEST(trainer, confianza_inicial_y_ganancias) {
  CHECK_EQ((int)startingTrust(20, 20, false), 100);
  CHECK_EQ((int)startingTrust(26, 20, false), 30);   // mas de 5 niveles por encima
  CHECK_EQ((int)startingTrust(10, 20, true), 40);    // intercambiado
  OwnedMon m;
  m.trust = 30;
  for (int i = 0; i < 10; i++) trustAfterBattle(m);
  CHECK_EQ((int)m.trust, 40);                         // tope de +10 al dia
  trustPerKm(m, 3);
  trustTournamentWin(m);
  CHECK_EQ((int)m.trust, 48);
}

TEST(trainer, captura_master_ball_y_probabilidad) {
  Rng rng;
  BattleMon wild;
  monInit(wild, SNORLAX, 30);
  uint8_t shakes = 0;
  CHECK(catchAttempt(rng, wild, IT_MASTER_BALL, 10, &shakes));
  CHECK_EQ((int)shakes, 3);
  // Pidgey con PS llenos y Poke Ball: 33 %
  BattleMon p;
  monInit(p, PIDGEY, 10);
  int ok = 0;
  for (int i = 0; i < 3000; i++) ok += catchAttempt(rng, p, IT_POKE_BALL, 10, nullptr);
  CHECK_RANGE(ok, 900, 1100);
}

TEST(trainer, capturados_llenan_equipo_y_luego_pc) {
  Trainer &t = *freshTrainer();
  Rng rng;
  BattleMon wild;
  monInit(wild, PIDGEY, 5);
  wild.hp = wild.maxHp / 2;
  for (int i = 0; i < 8; i++) CHECK(addCaught(t, wild, rng));
  CHECK_EQ((int)t.partyCount, 6);
  CHECK_EQ((int)t.boxCount, 2);
  CHECK(dexHas(t.caught, PIDGEY));
  CHECK_EQ((int)dexCount(t.caught), 1);
  // conserva la proporcion de PS (la mitad, con un margen por los genes nuevos)
  uint16_t maxHp = ownedMaxHp(t.party[0]);
  CHECK_RANGE((int)t.party[0].hp, (int)maxHp / 2 - 1, (int)maxHp / 2 + 1);
}

TEST(trainer, el_companero_no_se_deja_en_el_pc) {
  Trainer &t = *freshTrainer();
  Rng rng;
  OwnedMon partner, other;
  ownedInit(partner, PIKACHU, 10, rng);
  partner.flags |= OM_PARTNER;
  ownedInit(other, PIDGEY, 10, rng);
  addOwned(t, partner);
  addOwned(t, other);
  CHECK(!depositToBox(t, 0));
  CHECK(depositToBox(t, 1));
  CHECK_EQ((int)t.partyCount, 1);
  CHECK_EQ((int)t.boxCount, 1);
  CHECK(withdrawFromBox(t, 0));
  CHECK_EQ((int)t.party[1].species, PIDGEY);
}

TEST(trainer, objetos_fuera_de_combate) {
  Trainer &t = *freshTrainer();
  Rng rng;
  OwnedMon m;
  ownedInit(m, PIKACHU, 20, rng);
  uint16_t maxHp = ownedMaxHp(m);
  CHECK_EQ((int)useItemOn(t, IT_POTION, m), (int)USE_NO_ITEM);
  addItem(t, IT_POTION, 2);
  CHECK_EQ((int)useItemOn(t, IT_POTION, m), (int)USE_NO_EFFECT);  // PS llenos: no se gasta
  CHECK_EQ((int)t.bag[IT_POTION], 2);
  m.hp = 5;
  CHECK_EQ((int)useItemOn(t, IT_POTION, m), (int)USE_OK);
  CHECK_EQ((int)m.hp, 5 + 20 * HP_MULT > maxHp ? maxHp : 5 + 20 * HP_MULT);
  CHECK_EQ((int)t.bag[IT_POTION], 1);
  m.hp = 0;
  addItem(t, IT_REVIVE);
  CHECK_EQ((int)useItemOn(t, IT_REVIVE, m), (int)USE_OK);
  CHECK_EQ((int)m.hp, maxHp / 2);
  m.status = STS_PAR;
  addItem(t, IT_ANTIDOTE);
  CHECK_EQ((int)useItemOn(t, IT_ANTIDOTE, m), (int)USE_NO_EFFECT);
  addItem(t, IT_FULL_HEAL);
  CHECK_EQ((int)useItemOn(t, IT_FULL_HEAL, m), (int)USE_OK);
  CHECK_EQ((int)m.status, (int)STS_NONE);
  addItem(t, IT_THUNDER_STONE);
  CHECK_EQ((int)useItemOn(t, IT_THUNDER_STONE, m), (int)USE_EVOLVED);
  CHECK_EQ((int)m.species, RAICHU);
  addItem(t, IT_RARE_CANDY);
  LevelUp lv;
  CHECK_EQ((int)useItemOn(t, IT_RARE_CANDY, m, &lv), (int)USE_LEVEL_UP);
  CHECK_EQ((int)m.level, 21);
  addItem(t, IT_CALCIUM, 20);
  for (int i = 0; i < 10; i++) CHECK_EQ((int)useItemOn(t, IT_CALCIUM, m), (int)USE_OK);
  CHECK_EQ((int)useItemOn(t, IT_CALCIUM, m), (int)USE_NO_EFFECT);  // tope de 25600
}

TEST(trainer, dinero_medallas_y_tienda) {
  Trainer &t = *freshTrainer();
  CHECK_EQ((int)t.money, 3000);
  CHECK(!spend(t, 5000));
  CHECK(spend(t, 1000));
  blackout(t);
  CHECK_EQ((int)t.money, 1800);
  CHECK(martSells(t, IT_POKE_BALL));
  CHECK(!martSells(t, IT_GREAT_BALL));
  t.badges = 0b11;
  CHECK(martSells(t, IT_GREAT_BALL));
  CHECK(!martSells(t, IT_ULTRA_BALL));
  CHECK(!martSells(t, IT_MASTER_BALL));
  CHECK_EQ((int)prizeMoney(99, 50), 4950);
}

TEST(trainer, guardar_y_cargar_detecta_corrupcion) {
  Trainer &t = *freshTrainer();
  Rng rng;
  OwnedMon m;
  ownedInit(m, BULBASAUR, 12, rng);
  m.flags |= OM_PARTNER;
  addOwned(t, m);
  t.money = 4321;
  t.badges = 0b101;
  addItem(t, IT_ULTRA_BALL, 7);
  std::vector<uint8_t> buf(trainerSaveSize());
  CHECK_EQ((int)trainerSave(t, buf.data(), buf.size()), (int)buf.size());
  Trainer loaded;
  CHECK(trainerLoad(loaded, buf.data(), buf.size()));
  CHECK_EQ((int)loaded.money, 4321);
  CHECK_EQ((int)loaded.party[0].species, BULBASAUR);
  CHECK_EQ((int)loaded.bag[IT_ULTRA_BALL], 7);
  CHECK(memcmp(&loaded, &t, sizeof(Trainer)) == 0);
  buf[40] ^= 0x5A;  // un byte cambiado
  CHECK(!trainerLoad(loaded, buf.data(), buf.size()));
  CHECK(!trainerLoad(loaded, buf.data(), 10));
}

TEST(trainer, convertir_a_batalla_y_volver) {
  Rng rng;
  OwnedMon m;
  ownedInit(m, SNORLAX, 30, rng);
  m.hp = 50;
  m.status = STS_PAR;
  BattleMon b;
  toBattle(m, b);
  CHECK_EQ((int)b.hp, 50);
  CHECK_EQ((int)b.status, (int)STS_PAR);
  CHECK_EQ((int)b.maxHp, (int)ownedMaxHp(m));
  b.hp = 10;
  b.status = STS_TOX;
  b.pp[0] = 1;
  fromBattle(m, b);
  CHECK_EQ((int)m.hp, 10);
  CHECK_EQ((int)m.status, (int)STS_PSN);  // Toxico queda como veneno normal
  CHECK_EQ((int)m.pp[0], 1);
}
