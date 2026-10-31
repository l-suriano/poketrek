// Recompensas (rewards.cpp), tiendas (shop.cpp) y torneos (tournament.cpp).
#include "framework.h"
#include "shim/Arduino.h"
#include "../tournament.h"
#include "../shop.h"
#include "../gyms.h"
#include <string.h>

enum { BULBASAUR = 1, CHARMANDER = 4, CHARIZARD = 6, SQUIRTLE = 7, PIDGEY = 16, PIKACHU = 25,
       GROWLITHE = 58, PONYTA = 77, MAGNEMITE = 81, MEW = 151 };

static const uint32_t MONDAY = 1793577600u;  // lunes 2-11-2026 00:00
static const uint32_t DAY = 86400u;

static Trainer *freshTrainer() {
  static Trainer t;
  t = Trainer();
  return &t;
}

static void addMon(Trainer &t, uint8_t species, uint8_t level, bool box = false) {
  Rng rng;
  OwnedMon m;
  ownedInit(m, species, level, rng);
  if (box) t.box[t.boxCount++] = m;
  else addOwned(t, m);
}

static Trainer *teamOf3(uint8_t level) {
  Trainer *t = freshTrainer();
  addMon(*t, BULBASAUR, level);
  addMon(*t, CHARMANDER, level);
  addMon(*t, SQUIRTLE, level);
  t->money = 100000;
  return t;
}

static const TourPick FIRST3[TOUR_TEAM] = { {0, 0}, {0, 1}, {0, 2} };

// --------------------------------------------------------------- recompensas

TEST(rewards, semanas_empiezan_el_lunes) {
  CHECK(weekIndex(MONDAY - 1) != weekIndex(MONDAY));
  CHECK_EQ((int)weekIndex(MONDAY), (int)weekIndex(MONDAY + 7 * DAY - 1));
  CHECK(weekIndex(MONDAY + 7 * DAY) == weekIndex(MONDAY) + 1);
  Progress p;
  progressWeek(p, MONDAY);
  p.candiesBought = 3;
  p.featuredWins = 2;
  progressWeek(p, MONDAY + 3 * DAY);
  CHECK_EQ((int)p.candiesBought, 3);
  progressWeek(p, MONDAY + 7 * DAY);
  CHECK_EQ((int)p.candiesBought, 0);
  CHECK_EQ((int)p.featuredWins, 0);
}

TEST(rewards, bono_de_victoria_limpia) {
  CHECK_EQ((int)cleanWinBonus(1000, false, false), 1000);
  CHECK_EQ((int)cleanWinBonus(1000, true, false), 1250);
  CHECK_EQ((int)cleanWinBonus(1000, true, true), 1562);
}

TEST(rewards, gimnasio_medalla_mt_y_revanchas) {
  Trainer *t = freshTrainer();
  addMon(*t, PIDGEY, 20);
  t->money = 0;
  GymResult r = gymVictory(*t, 0, 20, 20, false);  // Brock con el as a 20
  CHECK(r.newBadge);
  CHECK(t->badges & 1);
  CHECK_EQ((int)r.prize.tm[0], 34);
  CHECK_EQ((int)t->tms[34 - 1], 1);
  CHECK_EQ((int)r.money, 99 * 20);
  CHECK(r.exp.firstLeaderWin);
  // repetir antes de ser Campeon no da nada
  r = gymVictory(*t, 0, 20, 20, false);
  CHECK(!r.newBadge);
  CHECK_EQ((int)r.money, 0);
  CHECK_EQ((int)t->tms[34 - 1], 1);
  // revancha tras ser Campeon: mitad de dinero, sin MT
  t->champion = true;
  r = gymVictory(*t, 0, 50, 50, false);
  CHECK(r.exp.rematch);
  CHECK_EQ((int)r.money, 99 * 50 / 2);
  CHECK_EQ((int)t->tms[26 - 1], 0);
  // premios con objeto
  GymPrize erika = gymPrize(3, 50);
  CHECK_EQ((int)erika.tm[0], 22);
  CHECK_EQ((int)erika.item, (int)IT_LEAF_STONE);
  GymPrize koga = gymPrize(4, 30);
  CHECK_EQ((int)koga.item, (int)IT_SUPER_POTION);
  CHECK_EQ((int)koga.itemCount, 3);
  GymPrize sabrina = gymPrize(5, 50);
  CHECK_EQ((int)sabrina.tm[1], 42);
}

TEST(rewards, master_ball_una_vez_del_campeon) {
  Trainer *t = freshTrainer();
  Progress p;
  eliteVictory(*t, p, 0);
  CHECK(!t->champion);
  CHECK(p.eliteBeaten & 1);
  eliteVictory(*t, p, ELITE_CHAMPION);
  CHECK(t->champion);
  CHECK_EQ((int)t->bag[IT_MASTER_BALL], 1);
  eliteVictory(*t, p, ELITE_CHAMPION);
  CHECK_EQ((int)t->bag[IT_MASTER_BALL], 1);
}

TEST(rewards, metas_del_daily_challenge) {
  Trainer *t = freshTrainer();
  addMon(*t, PIDGEY, 30);
  Progress p;
  Explore ex;
  Rng rng;
  CHECK(dailyCountDay(p, *t, ex, rng, MONDAY));
  CHECK(!dailyCountDay(p, *t, ex, rng, MONDAY + 3600));  // un intento al dia
  CHECK(dailyPlayedToday(p, MONDAY + 3600));
  for (int d = 1; d < 7; d++) dailyCountDay(p, *t, ex, rng, MONDAY + d * DAY);
  CHECK_EQ((int)p.dcDays, 7);
  CHECK_EQ((int)p.dcStreak, 7);
  CHECK_EQ((int)t->bp, 5);                       // 7 dias seguidos
  CHECK_EQ((int)ex.eggs[0].tier, (int)EGG_RARE);  // Huevo Raro
  // un hueco no rompe las metas: cuentan dias totales
  for (int d = 8; d < 15; d++) dailyCountDay(p, *t, ex, rng, MONDAY + d * DAY);
  CHECK_EQ((int)p.dcDays, 14);
  CHECK_EQ((int)p.dcStreak, 7);
  CHECK(p.skins & SKIN_GREAT);
  CHECK_EQ((int)wildShinyDen(p), 256);
  // Mew a los 30 con el equipo y el PC llenos: espera y llega despues
  while (t->partyCount < PARTY_MAX) addMon(*t, PIDGEY, 5);
  while (t->boxCount < BOX_MAX) addMon(*t, PIDGEY, 5, true);
  for (int d = 15; p.dcDays < 30; d++) dailyCountDay(p, *t, ex, rng, MONDAY + d * DAY);
  CHECK(p.mewPending);
  CHECK(!p.mewGiven);
  CHECK_EQ((int)wildShinyDen(p), 128);
  t->boxCount--;
  deliverPending(p, *t, ex, rng);
  CHECK(p.mewGiven);
  CHECK_EQ((int)t->box[BOX_MAX - 1].species, MEW);
  CHECK_EQ((int)t->box[BOX_MAX - 1].trust, 100);
  // a los 100: huevo shiny (cuando haya hueco), skin y titulo
  for (int i = 0; i < MAX_EGGS; i++) ex.eggs[i] = Egg();
  for (int d = 40; p.dcDays < 100; d++) dailyCountDay(p, *t, ex, rng, MONDAY + d * DAY);
  CHECK(p.titles & TITLE_KANTO_LEGEND);
  CHECK(p.skins & SKIN_MASTER);
  bool shinyEgg = false;
  for (int i = 0; i < MAX_EGGS; i++) shinyEgg |= ex.eggs[i].species && ex.eggs[i].shiny;
  CHECK(shinyEgg);
}

TEST(rewards, guardar_y_cargar_progreso) {
  Progress p;
  p.dcDays = 42;
  p.ribbons = 5;
  static uint8_t buf[512];
  size_t n = progressSave(p, buf, sizeof(buf));
  CHECK_EQ((int)n, (int)progressSaveSize());
  Progress back;
  CHECK(progressLoad(back, buf, n));
  CHECK_EQ((int)back.dcDays, 42);
  buf[SAVE_HEADER] ^= 1;
  CHECK(!progressLoad(back, buf, n));
}

// ------------------------------------------------------------------ tiendas -

TEST(shop, poke_mart_segun_medallas) {
  Trainer *t = freshTrainer();
  t->money = 1000;
  CHECK_EQ((int)martBuy(*t, IT_POKE_BALL, 3), (int)SHOP_OK);
  CHECK_EQ((int)t->money, 400);
  CHECK_EQ((int)t->bag[IT_POKE_BALL], 3);
  CHECK_EQ((int)martBuy(*t, IT_GREAT_BALL), (int)SHOP_NOT_SOLD);  // 2 medallas
  CHECK_EQ((int)martBuy(*t, IT_POTION, 2), (int)SHOP_NO_MONEY);
  t->badges = 0x03;
  t->money = 100000;
  CHECK_EQ((int)martBuy(*t, IT_GREAT_BALL), (int)SHOP_OK);
  CHECK_EQ((int)martBuy(*t, IT_MASTER_BALL), (int)SHOP_NOT_SOLD);
  t->bag[IT_POKE_BALL] = 98;
  CHECK_EQ((int)martBuy(*t, IT_POKE_BALL, 2), (int)SHOP_BAG_FULL);
}

TEST(shop, vender_a_mitad_de_precio) {
  Trainer *t = freshTrainer();
  t->money = 0;
  addItem(*t, IT_NUGGET, 2);
  addItem(*t, IT_EXP_SHARE);
  CHECK_EQ((int)sellPrice(IT_POTION), 150);
  CHECK_EQ((int)sellItem(*t, IT_NUGGET, 2), (int)SHOP_OK);
  CHECK_EQ((int)t->money, 10000);  // 5000 cada una
  CHECK_EQ((int)sellItem(*t, IT_NUGGET), (int)SHOP_LIMIT);
  CHECK_EQ((int)sellItem(*t, IT_EXP_SHARE), (int)SHOP_NOT_SOLD);
  CHECK_EQ((int)sellItem(*t, IT_RARE_CANDY), (int)SHOP_NOT_SOLD);
}

TEST(shop, celadon_desde_la_medalla_de_erika) {
  Trainer *t = freshTrainer();
  t->money = 50000;
  CHECK_EQ((int)celadonBuy(*t, IT_FIRE_STONE), (int)SHOP_CLOSED);
  t->badges = 0x0F;
  CHECK_EQ((int)celadonBuy(*t, IT_FIRE_STONE), (int)SHOP_OK);
  CHECK_EQ((int)t->money, 50000 - 2100);
  CHECK_EQ((int)celadonBuy(*t, IT_MOON_STONE), (int)SHOP_NOT_SOLD);
  CHECK_EQ((int)celadonBuy(*t, IT_PROTEIN), (int)SHOP_OK);
  uint8_t a[CELADON_TMS], b[CELADON_TMS], c[CELADON_TMS];
  celadonTms(100, a);
  celadonTms(100, b);
  CHECK_EQ(0, memcmp(a, b, CELADON_TMS));  // igual para todos esa semana
  CHECK(a[0] != a[1] && a[1] != a[2] && a[0] != a[2]);
  bool changes = false;
  for (int w = 101; w < 110; w++) { celadonTms((uint16_t)w, c); changes |= memcmp(a, c, CELADON_TMS) != 0; }
  CHECK(changes);
  for (int i = 0; i < CELADON_TMS; i++) CHECK(tmPrice(a[i]) > 0);
  uint32_t before = t->money;
  CHECK_EQ((int)celadonBuyTm(*t, 100, 1), (int)SHOP_OK);
  CHECK_EQ((int)t->tms[a[1] - 1], 1);
  CHECK_EQ((int)(before - t->money), (int)tmPrice(a[1]));
}

TEST(shop, battle_shop_con_bp_y_topes) {
  Trainer *t = freshTrainer();
  Progress p;
  t->bp = 200;
  for (int i = 0; i < 3; i++) CHECK_EQ((int)battleShopBuy(*t, p, 0, MONDAY), (int)SHOP_OK);
  CHECK_EQ((int)battleShopBuy(*t, p, 0, MONDAY + DAY), (int)SHOP_LIMIT);  // 3 a la semana
  CHECK_EQ((int)t->bag[IT_RARE_CANDY], 3);
  CHECK_EQ((int)battleShopBuy(*t, p, 0, MONDAY + 7 * DAY), (int)SHOP_OK);  // semana nueva
  CHECK_EQ((int)battleShopBuy(*t, p, 3, MONDAY), (int)SHOP_OK);  // Repartir Exp.
  CHECK_EQ((int)battleShopBuy(*t, p, 3, MONDAY), (int)SHOP_LIMIT);
  CHECK_EQ((int)t->bp, 200 - 80 - 50);
  t->bp = 10;
  CHECK_EQ((int)battleShopBuy(*t, p, 5, MONDAY), (int)SHOP_NO_BP);
  t->bp = 24;
  CHECK_EQ((int)battleShopBuy(*t, p, 5, MONDAY), (int)SHOP_OK);
  CHECK_EQ((int)t->tms[BATTLE_SHOP[5].id - 1], 1);
  // las MT de la Battle Shop no se venden en Celadon
  for (int i = 0; i < NUM_BATTLE_SHOP; i++)
    if (BATTLE_SHOP[i].kind == BS_TM) CHECK_EQ((int)tmPrice(BATTLE_SHOP[i].id), 0);
  for (int i = 0; i < NUM_UNSOLD_TMS; i++) CHECK_EQ((int)tmPrice(UNSOLD_TMS[i]), 0);
}

// ----------------------------------------------------------------- torneos --

TEST(tour, cuotas_y_featured_cup) {
  CHECK_EQ((int)tourFee(TT_ROOKIE, false), 500);
  CHECK_EQ((int)tourFee(TT_MASTER, false), 10000);
  CHECK_EQ((int)tourFee(TT_GREAT, true), 1500);  // 25 % menos
  Featured f = featuredCup(500);
  Featured g = featuredCup(500);
  CHECK_EQ((int)f.cup, (int)g.cup);
  CHECK(f.cup < CUP_CHAMPION);
  int seen[NUM_CUPS] = {};
  for (int w = 0; w < 300; w++) {
    Featured x = featuredCup((uint16_t)w);
    seen[x.cup]++;
    CHECK((x.cup == CUP_MONO) == (x.type != T_NONE));
  }
  for (int c = 0; c < CUP_CHAMPION; c++) CHECK(seen[c] > 0);
  CHECK_EQ(seen[CUP_CHAMPION], 0);
}

TEST(tour, reglas_de_equipo) {
  Trainer *t = teamOf3(20);
  TourPick dup[TOUR_TEAM] = { {0, 0}, {0, 0}, {0, 1} };
  CHECK_EQ((int)tourCheckTeam(*t, dup, false, T_NONE), (int)TOUR_DUPLICATE);
  TourPick bad[TOUR_TEAM] = { {0, 0}, {0, 1}, {1, 0} };
  CHECK_EQ((int)tourCheckTeam(*t, bad, false, T_NONE), (int)TOUR_BAD_PICK);
  CHECK_EQ((int)tourCheckTeam(*t, FIRST3, true, T_NONE), (int)TOUR_OK);  // los 3 aun evolucionan
  addMon(*t, CHARIZARD, 40);
  TourPick zard[TOUR_TEAM] = { {0, 0}, {0, 1}, {0, 3} };
  CHECK_EQ((int)tourCheckTeam(*t, zard, true, T_NONE), (int)TOUR_RULE);
  CHECK_EQ((int)tourCheckTeam(*t, FIRST3, false, T_FIRE), (int)TOUR_RULE);
}

TEST(tour, inscripcion_paga_y_escala) {
  Trainer *t = teamOf3(30);
  Progress p;
  Tournament run;
  Rng rng;
  uint32_t now = MONDAY;
  uint8_t cup = featuredCup(weekIndex(now)).cup == CUP_OPEN ? CUP_NO_ITEMS : CUP_OPEN;
  CHECK_EQ((int)tourEnter(run, *t, p, cup, TT_ULTRA, FIRST3, now, rng), (int)TOUR_OK);
  CHECK_EQ((int)t->money, 100000 - 5000);
  CHECK_EQ((int)run.oppLevel, 33);  // Base Level 30 + 3
  CHECK_EQ((int)tourEnter(run, *t, p, CUP_CHAMPION, TT_MASTER, FIRST3, now, rng), (int)TOUR_LOCKED);
  t->champion = true;
  CHECK_EQ((int)tourEnter(run, *t, p, CUP_CHAMPION, TT_GREAT, FIRST3, now, rng), (int)TOUR_LOCKED);
  CHECK_EQ((int)tourEnter(run, *t, p, CUP_RANDOM, TT_GREAT, FIRST3, now, rng), (int)TOUR_NOT_ENOUGH);
  t->money = 100;
  CHECK_EQ((int)tourEnter(run, *t, p, cup, TT_GREAT, FIRST3, now, rng), (int)TOUR_NO_MONEY);
}

TEST(tour, niveles_por_copa) {
  Trainer *t = teamOf3(40);
  Progress p;
  Tournament run;
  Rng rng;
  BattleSide side;
  CHECK_EQ((int)tourEnter(run, *t, p, CUP_L30, TT_GREAT, FIRST3, MONDAY, rng), (int)TOUR_OK);
  tourPlayerSide(run, *t, side);
  CHECK_EQ((int)side.count, 3);
  CHECK_EQ((int)side.party[0].level, 30);
  CHECK_EQ((int)side.party[0].hp, (int)side.party[0].maxHp);
  CHECK_EQ((int)run.oppLevel, 30);
  CHECK_EQ((int)tourEnter(run, *t, p, CUP_FLAT50, TT_GREAT, FIRST3, MONDAY, rng), (int)TOUR_OK);
  tourPlayerSide(run, *t, side);
  CHECK_EQ((int)side.party[2].level, 50);
  CHECK_EQ((int)tourExpFoeLevel(run, 40, 50), 40);  // la XP usa tu nivel real
  // el equipo real no cambia
  CHECK_EQ((int)t->party[0].level, 40);
  Trainer *low = teamOf3(12);
  CHECK_EQ((int)tourEnter(run, *low, p, CUP_L30, TT_GREAT, FIRST3, MONDAY, rng), (int)TOUR_OK);
  tourPlayerSide(run, *low, side);
  CHECK_EQ((int)side.party[0].level, 12);  // por debajo de 30 se queda igual
}

TEST(tour, rivales_deterministas_y_segun_categoria) {
  Trainer *t = teamOf3(30);
  Progress p;
  Rng rng;
  BattleSide me, a, b;
  uint8_t ai[NUM_TOUR_TIERS];
  for (int tier = 0; tier < NUM_TOUR_TIERS; tier++) {
    Tournament run;
    uint8_t cup = featuredCup(weekIndex(MONDAY)).cup == CUP_OPEN ? CUP_NO_ITEMS : CUP_OPEN;
    CHECK_EQ((int)tourEnter(run, *t, p, cup, (uint8_t)tier, FIRST3, MONDAY, rng), (int)TOUR_OK);
    run.special = SPC_NONE;
    tourPlayerSide(run, *t, me);
    tourOpponent(run, me, a);
    tourOpponent(run, me, b);
    CHECK_EQ((int)a.count, 3);
    for (int i = 0; i < 3; i++) CHECK_EQ((int)a.party[i].species, (int)b.party[i].species);
    CHECK_EQ((int)a.party[2].level, (int)run.oppLevel);
    CHECK(a.party[0].species != a.party[1].species);
    ai[tier] = a.aiTier;
  }
  CHECK(ai[TT_ROOKIE] < ai[TT_GREAT]);
  CHECK(ai[TT_GREAT] < ai[TT_ULTRA]);
  CHECK_EQ((int)ai[TT_MASTER], 4);
}

TEST(tour, rivales_respetan_monotipo_y_copa_pequena) {
  Trainer *t = freshTrainer();
  addMon(*t, GROWLITHE, 25);
  addMon(*t, PONYTA, 25);
  addMon(*t, CHARMANDER, 25);
  t->money = 100000;
  Progress p;
  Rng rng;
  BattleSide me, opp;
  Tournament run;
  Featured f = featuredCup(weekIndex(MONDAY));
  if (f.cup != CUP_MONO) {
    CHECK_EQ((int)tourEnter(run, *t, p, CUP_MONO, TT_GREAT, FIRST3, MONDAY, rng), (int)TOUR_OK);
    CHECK_EQ((int)run.monoType, (int)T_FIRE);
    tourPlayerSide(run, *t, me);
    for (int r = 0; r < 3; r++) {
      run.wins = (uint8_t)r;
      run.special = SPC_NONE;
      tourOpponent(run, me, opp);
      for (int i = 0; i < 3; i++) CHECK(hasType(opp.party[i].species, T_FIRE));
    }
  }
  if (f.cup != CUP_LITTLE) {
    CHECK_EQ((int)tourEnter(run, *t, p, CUP_LITTLE, TT_GREAT, FIRST3, MONDAY, rng), (int)TOUR_OK);
    tourPlayerSide(run, *t, me);
    for (int r = 0; r < 3; r++) {
      run.wins = (uint8_t)r;
      tourOpponent(run, me, opp);
      for (int i = 0; i < 3; i++) {
        CHECK(canStillEvolve(opp.party[i].species));
        CHECK_EQ((int)opp.party[i].level, 20);
      }
    }
  }
  CHECK_EQ((int)tourEnter(run, *t, p, CUP_NO_ITEMS, TT_MASTER, FIRST3, MONDAY, rng), (int)TOUR_OK);
  tourOpponent(run, me, opp);
  CHECK_EQ((int)opp.potions, 0);
  CHECK_EQ((int)tourItemsAllowed(run), 0);
}

static TourPayout playOut(Trainer &t, Progress &p, Explore &ex, uint8_t tier, int wins, bool featured,
                          uint32_t now) {
  Tournament run;
  Rng rng;
  Featured f = featuredCup(weekIndex(now));
  uint8_t cup = featured ? f.cup : (uint8_t)(f.cup == CUP_OPEN ? CUP_NO_ITEMS : CUP_OPEN);
  if (featured && (cup == CUP_LITTLE || cup == CUP_MONO || cup == CUP_RANDOM)) cup = CUP_OPEN;  // sin regla
  tourEnter(run, t, p, cup, tier, FIRST3, now, rng);
  run.special = SPC_NONE;
  for (int r = 0; r < 3 && !tourOver(run); r++) tourReport(run, r < wins, false, 50, 10);
  return tourFinish(run, t, p, ex, now);
}

TEST(tour, premios_por_ronda) {
  Trainer *t = teamOf3(30);
  Progress p;
  Explore ex;
  uint32_t m0 = t->money;
  TourPayout r = playOut(*t, p, ex, TT_GREAT, 0, false, MONDAY);
  CHECK_EQ((int)r.money, 0);
  CHECK_EQ((int)(m0 - t->money), 2000);
  r = playOut(*t, p, ex, TT_GREAT, 1, false, MONDAY);
  CHECK_EQ((int)r.money, 2000);  // semifinal: devuelve la cuota
  r = playOut(*t, p, ex, TT_GREAT, 2, false, MONDAY);
  CHECK_EQ((int)r.money, 3000);
  CHECK_EQ((int)r.bp, 2);
  CHECK(r.finalist);
  uint8_t trust = t->party[0].trust = 50;
  r = playOut(*t, p, ex, TT_GREAT, 3, false, MONDAY);
  CHECK(r.champion);
  CHECK_EQ((int)r.money, 6000);
  CHECK_EQ((int)r.bp, 6);
  CHECK_EQ((int)t->party[0].trust, trust + 5);
  CHECK_EQ((int)p.tournamentsWon, 1);
}

TEST(tour, featured_cup_bp_doble_y_premio_semanal) {
  Featured f = featuredCup(weekIndex(MONDAY));
  if (f.cup == CUP_LITTLE || f.cup == CUP_MONO || f.cup == CUP_RANDOM) return;  // reglas de equipo aparte
  Trainer *t = teamOf3(30);
  Progress p;
  Explore ex;
  uint32_t m0 = t->money;
  TourPayout r = playOut(*t, p, ex, TT_GREAT, 3, true, MONDAY);
  CHECK_EQ((int)r.bp, 12);
  CHECK_EQ((int)(t->money - m0), 6000 - 1500);  // cuota con descuento, premio entero
  CHECK_EQ((int)r.weeklyKind, (int)PZ_NONE);
  playOut(*t, p, ex, TT_ROOKIE, 3, true, MONDAY + DAY);
  r = playOut(*t, p, ex, TT_ROOKIE, 3, true, MONDAY + 2 * DAY);
  CHECK(r.weeklyKind != PZ_NONE);  // 3 victorias esa semana
  r = playOut(*t, p, ex, TT_ROOKIE, 3, true, MONDAY + 3 * DAY);
  CHECK_EQ((int)r.weeklyKind, (int)PZ_NONE);  // una vez por semana
}

TEST(tour, retador_especial_da_mas_bp) {
  Trainer *t = teamOf3(30);
  t->badges = 0xFF;
  Progress p;
  Explore ex;
  Rng rng;
  int specials = 0;
  for (int i = 0; i < 400; i++) {
    Tournament run;
    t->money = 100000;
    uint8_t cup = featuredCup(weekIndex(MONDAY)).cup == CUP_OPEN ? CUP_NO_ITEMS : CUP_OPEN;
    tourEnter(run, *t, p, cup, TT_GREAT, FIRST3, MONDAY, rng);
    if (run.special == SPC_NONE) continue;
    specials++;
    BattleSide me, opp;
    tourPlayerSide(run, *t, me);
    run.wins = 2;
    TourRival who = tourRival(run);
    CHECK_EQ((int)who.special, (int)run.special);
    tourOpponent(run, me, opp);
    CHECK_EQ((int)opp.aiTier, 4);
    run.wins = 0;
    for (int r = 0; r < 3; r++) tourReport(run, true, false, 50, 10);
    TourPayout pay = tourFinish(run, *t, p, ex, MONDAY);
    CHECK(pay.specialBeaten);
    CHECK_EQ((int)pay.bp, 9);  // 6 BP + 50 %
  }
  CHECK(specials > 40 && specials < 85);  // 5 % Blue + 10 % lider vencido
}

TEST(tour, daily_challenge_igual_para_todos_y_un_intento) {
  Trainer *a = teamOf3(30), b = *a;
  b.party[0].species = PIKACHU;  // otro jugador con otro equipo
  Progress pa, pb;
  Explore ex;
  Rng rng;
  uint32_t now = MONDAY + 3 * DAY + 3600;
  DailyChallenge dc = dailyChallenge(dayIndex(now));
  // elegir un dia sin regla de equipo para que ambos equipos valgan
  while (dc.rule == DR_MONO || dc.rule == DR_LITTLE) { now += DAY; dc = dailyChallenge(dayIndex(now)); }
  Tournament ra, rb;
  CHECK_EQ((int)dailyEnter(ra, *a, pa, ex, FIRST3, now, rng), (int)TOUR_OK);
  CHECK_EQ((int)dailyEnter(rb, b, pb, ex, FIRST3, now, rng), (int)TOUR_OK);
  CHECK_EQ((int)dailyEnter(ra, *a, pa, ex, FIRST3, now + 60, rng), (int)TOUR_DAILY_DONE);
  CHECK_EQ((int)pa.dcDays, 1);
  BattleSide sa, sb, oa, ob;
  tourPlayerSide(ra, *a, sa);
  tourPlayerSide(rb, b, sb);
  CHECK_EQ((int)sa.party[0].level, 50);
  for (int r = 0; r < 3; r++) {
    ra.wins = rb.wins = (uint8_t)r;
    tourOpponent(ra, sa, oa);
    tourOpponent(rb, sb, ob);
    for (int i = 0; i < 3; i++) {
      CHECK_EQ((int)oa.party[i].species, (int)ob.party[i].species);
      CHECK_EQ((int)oa.party[i].level, 50);
      for (int k = 0; k < 4; k++) CHECK_EQ((int)oa.party[i].moves[k], (int)ob.party[i].moves[k]);
    }
  }
  ra.wins = 0;
  uint32_t money = a->money;
  for (int r = 0; r < 3; r++) tourReport(ra, true, false, 80, 12);
  TourPayout pay = tourFinish(ra, *a, pa, ex, now);
  CHECK(pay.champion);
  CHECK(pay.perfect);
  CHECK_EQ((int)pay.bp, 18);  // 12 BP + 50 % por Perfect Run
  CHECK_EQ((int)pay.money, 0);
  CHECK_EQ((int)a->money, (int)money);
  CHECK(pa.perfectCrown);
  CHECK_EQ((int)pa.dcScore, 3 * (1000 + 400 - 120));
  CHECK(pay.prizeKind != PZ_NONE);
  CHECK(tourExpBonus(ra).featured);
}

TEST(tour, torneo_completo_con_el_motor) {
  // juega torneos de verdad con la IA en los dos lados: termina y paga lo que toca
  Trainer *t = freshTrainer();
  addMon(*t, 3, 40);   // Venusaur
  addMon(*t, 6, 40);   // Charizard
  addMon(*t, 9, 40);   // Blastoise
  t->money = 1000000;
  Progress p;
  Explore ex;
  Rng rng;
  rng.seed(77);
  int battles = 0, wins = 0;
  for (int n = 0; n < 30; n++) {
    Tournament run;
    uint8_t cup = featuredCup(weekIndex(MONDAY)).cup == CUP_OPEN ? CUP_NO_ITEMS : CUP_OPEN;
    CHECK_EQ((int)tourEnter(run, *t, p, cup, TT_ROOKIE, FIRST3, MONDAY, rng), (int)TOUR_OK);
    int played = 0;
    while (!tourOver(run)) {
      Battle b;
      battleInit(b, rng.next(), false);
      tourPlayerSide(run, *t, b.side[0]);
      b.side[0].aiTier = 3;
      tourOpponent(run, b.side[0], b.side[1]);
      while (b.outcome == OUT_NONE && b.turn < 500) {
        battleTurn(b, battleAiChoose(b, 0), battleAiChoose(b, 1));
        for (int s = 0; s < 2; s++)
          if (b.outcome == OUT_NONE && battleNeedsSwitch(b, s)) battleSwitchIn(b, s, battleAiPickSwitch(b, s));
      }
      CHECK(b.outcome != OUT_NONE);
      bool won = b.outcome == OUT_WIN0;
      int hp = 0, maxHp = 0;
      bool fainted = false;
      for (int i = 0; i < 3; i++) {
        hp += b.side[0].party[i].hp;
        maxHp += b.side[0].party[i].maxHp;
        fainted |= b.side[0].party[i].hp == 0;
      }
      tourReport(run, won, fainted, (uint8_t)(hp * 100 / maxHp), b.turn);
      played++;
      wins += won;
    }
    CHECK(played >= 1 && played <= TOUR_ROUNDS);
    battles += played;
    TourPayout pay = tourFinish(run, *t, p, ex, MONDAY);
    uint32_t fee = TOUR_TIERS[TT_ROOKIE].fee;
    uint32_t expect = run.wins == 3 ? fee * 3 : run.wins == 2 ? fee * 3 / 2 : run.wins == 1 ? fee : 0;
    CHECK_EQ((int)pay.money, (int)expect);
  }
  CHECK(battles >= 30);
  CHECK(wins > 0);
}
