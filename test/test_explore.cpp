// Exploracion (explore.cpp): pasos y EE, eventos afuera, rutas de casa,
// PokeStops, huevos, incienso, legendarios y guardado.
#include "framework.h"
#include "shim/Arduino.h"
#include "../explore.h"
#include <string.h>

enum { BULBASAUR = 1, PIDGEY = 16, PIDGEOTTO = 17, PIDGEOT = 18, ABRA = 63, KADABRA = 64,
       EEVEE = 133, ARTICUNO = 144, DRATINI = 147, MEWTWO = 150, MEW = 151 };
enum { R_ROUTE1 = 0, R_MTMOON = 2, R_SEAFOAM = 5, R_CERULEAN_CAVE = 8 };

static const uint32_t T0 = 1793000000u;  // una hora cualquiera de 2026

// entrenador con un equipo de `n` Pokemon a `level`
static Trainer *trainerAt(uint8_t level, uint8_t n = 1) {
  static Trainer t;
  t = Trainer();
  Rng rng;
  for (int i = 0; i < n; i++) {
    OwnedMon m;
    ownedInit(m, PIDGEY, level, rng);
    addOwned(t, m);
  }
  return &t;
}

static bool isBaseForm(uint8_t s) {
  for (int i = 0; i < NUM_EVOS; i++) if (EVO_TBL[i].to == s) return false;
  return true;
}

TEST(explore, tablas_de_rutas_y_rarezas) {
  for (int s = 1; s <= 151; s++)
    if (WILD_TIER[s]) CHECK(isBaseForm((uint8_t)s));
  for (int s : {144, 145, 146, 150, 151}) CHECK_EQ((int)WILD_TIER[s], (int)WT_NONE);
  CHECK_EQ((int)WILD_TIER[DRATINI], (int)WT_RARE);
  for (int r = 0; r < NUM_ROUTES; r++) {
    int total = 0;
    for (int i = 0; i < ROUTE_POOL && ROUTES[r].pool[i].weight; i++) {
      uint8_t s = ROUTES[r].pool[i].species;
      // en casa solo comunes y medias: las raras son de la zona lejana
      CHECK(WILD_TIER[s] == WT_COMMON || WILD_TIER[s] == WT_MID);
      total += ROUTES[r].pool[i].weight;
    }
    CHECK(total > 0);
  }
  CHECK_EQ((int)ROUTES[R_SEAFOAM].legend, ARTICUNO);
  CHECK_EQ((int)ROUTES[R_CERULEAN_CAVE].legend, MEWTWO);
}

TEST(explore, rutas_se_abren_con_medallas) {
  Trainer *t = trainerAt(10);
  CHECK(routeOpen(*t, R_ROUTE1));
  CHECK(!routeOpen(*t, R_MTMOON));
  t->badges = 1;
  CHECK(routeOpen(*t, R_MTMOON));
  t->badges = 0xFF;
  CHECK(routeOpen(*t, R_SEAFOAM));
  CHECK(!routeOpen(*t, R_CERULEAN_CAVE));
  t->champion = true;
  CHECK(routeOpen(*t, R_CERULEAN_CAVE));
}

TEST(explore, pasos_llenan_el_banco_y_dan_eventos_afuera) {
  Trainer *t = trainerAt(10);
  Explore ex;
  WalkInput in;
  in.steps = 1000; in.outside = true; in.now = T0;
  WalkResult r = exploreWalk(ex, *t, in);
  CHECK_EQ((int)r.ee, 1000);
  CHECK_EQ((int)r.events, 2);     // 400 y 800
  CHECK_EQ((int)r.drops, 1);      // sin GPS: Supply Drop cada 1000
  CHECK_EQ((int)ex.meters, 700);  // 0,7 m por paso
  r = exploreWalk(ex, *t, in);
  CHECK_EQ((int)r.events, 3);     // 1200, 1600, 2000
  // en casa suma EE pero no hay eventos
  in.outside = false;
  r = exploreWalk(ex, *t, in);
  CHECK_EQ((int)r.events, 0);
  CHECK_EQ((int)r.drops, 0);
  // el banco se llena a 5000
  for (int i = 0; i < 5; i++) exploreWalk(ex, *t, in);
  CHECK_EQ((int)ex.ee, EE_MAX);
}

TEST(explore, con_gps_no_hay_supply_drops_y_la_distancia_es_la_del_gps) {
  Trainer *t = trainerAt(10);
  Explore ex;
  WalkInput in;
  in.steps = 2000; in.meters = 1500; in.outside = true; in.gps = true; in.now = T0;
  WalkResult r = exploreWalk(ex, *t, in);
  CHECK_EQ((int)r.drops, 0);
  CHECK_EQ((int)ex.meters, 1500);
  CHECK_EQ((int)r.km, 1);
}

TEST(explore, en_coche_no_cuenta_nada) {
  Trainer *t = trainerAt(10);
  Explore ex;
  WalkInput in;
  in.steps = 1000; in.outside = true; in.speedKmh = 40; in.now = T0;
  WalkResult r = exploreWalk(ex, *t, in);
  CHECK_EQ((int)r.ee, 0);
  CHECK_EQ((int)r.events, 0);
  CHECK_EQ((int)ex.meters, 0);
}

TEST(explore, pasos_sueltos_no_pierden_milimetros) {
  Trainer *t = trainerAt(10);
  Explore ex;
  WalkInput in;
  in.steps = 1; in.now = T0;
  for (int i = 0; i < 10; i++) exploreWalk(ex, *t, in);
  CHECK_EQ((int)ex.meters, 7);
}

TEST(explore, km_suben_confianza_y_dan_un_caramelo_al_dia) {
  Trainer *t = trainerAt(10);
  t->party[0].trust = 30;
  Explore ex;
  WalkInput in;
  in.gps = true; in.meters = 10000; in.now = T0;
  WalkResult r = exploreWalk(ex, *t, in);
  CHECK_EQ((int)r.km, 10);
  CHECK_EQ((int)t->party[0].trust, 40);
  CHECK_EQ((int)r.candies, 1);
  CHECK_EQ((int)t->bag[IT_RARE_CANDY], 1);
  // otros 10 km el mismo dia: tope diario
  r = exploreWalk(ex, *t, in);
  CHECK_EQ((int)r.candies, 0);
  in.now = T0 + 86400;
  r = exploreWalk(ex, *t, in);
  CHECK_EQ((int)r.candies, 1);
}

TEST(explore, huevos_avanzan_y_eclosionan) {
  Trainer *t = trainerAt(30);
  Explore ex;
  Rng rng;
  CHECK(eggAdd(ex, rng, EGG_COMMON, 1));  // shinyDen 1: siempre shiny
  CHECK(eggAdd(ex, rng, EGG_RARE));
  CHECK(eggAdd(ex, rng, EGG_VERY_RARE));
  CHECK(!eggAdd(ex, rng, EGG_COMMON));    // maximo 3
  CHECK_EQ((int)WILD_TIER[ex.eggs[0].species], (int)WT_COMMON);
  CHECK_EQ((int)WILD_TIER[ex.eggs[2].species], (int)WT_RARE);
  CHECK(!eggReady(ex, 0));
  WalkInput in;
  in.gps = true; in.meters = 2000; in.now = T0;
  WalkResult r = exploreWalk(ex, *t, in);
  CHECK_EQ((int)r.eggsReady, 1);
  CHECK(eggReady(ex, 0));
  CHECK(!eggReady(ex, 1));
  uint8_t sp = ex.eggs[0].species;
  OwnedMon m;
  CHECK(eggHatch(ex, *t, rng, 0, &m));
  CHECK_EQ((int)m.species, (int)sp);
  CHECK_EQ((int)m.level, 25);  // Base Level 30 - 5
  CHECK_EQ((int)m.trust, 100);
  CHECK(m.flags & OM_SHINY);
  CHECK(dexHas(t->caught, sp));
  CHECK_EQ((int)ex.eggs[0].species, 0);
  CHECK(eggAdd(ex, rng, EGG_COMMON));  // el hueco queda libre
  // a nivel bajo nace al 5
  Trainer *low = trainerAt(6);
  in.meters = 10000;
  exploreWalk(ex, *low, in);
  CHECK(eggHatch(ex, *low, rng, 2, &m));
  CHECK_EQ((int)m.level, 5);
}

TEST(explore, huevos_nunca_traen_legendarios) {
  Rng rng;
  rng.seed(7);
  for (int i = 0; i < 2000; i++) {
    Explore ex;
    eggAdd(ex, rng, (uint8_t)(1 + i % 3));
    uint8_t s = ex.eggs[0].species;
    CHECK(s != 144 && s != 145 && s != 146 && s != MEWTWO && s != MEW);
    CHECK(WILD_TIER[s] != WT_NONE);
  }
}

TEST(explore, salvajes_evolucionan_solo_por_nivel) {
  CHECK_EQ((int)wildForm(PIDGEY, 10), PIDGEY);
  CHECK_EQ((int)wildForm(PIDGEY, 20), PIDGEOTTO);
  CHECK_EQ((int)wildForm(PIDGEY, 40), PIDGEOT);
  CHECK_EQ((int)wildForm(ABRA, 50), KADABRA);  // Alakazam es por intercambio
  CHECK_EQ((int)wildForm(EEVEE, 60), EEVEE);   // por piedra
}

TEST(explore, busqueda_en_ruta_gasta_ee_y_respeta_niveles) {
  Trainer *t = trainerAt(20);
  Explore ex;
  Rng rng;
  rng.seed(3);
  Encounter e;
  CHECK(!exploreSearchRoute(ex, *t, rng, R_ROUTE1, T0, e));  // sin EE
  CHECK(!exploreSearchRoute(ex, *t, rng, R_MTMOON, T0, e));  // cerrada
  int kinds[5] = {};
  for (int i = 0; i < 3000; i++) {
    ex.ee = EE_PER_SEARCH;
    CHECK(exploreSearchRoute(ex, *t, rng, R_ROUTE1, T0, e));
    CHECK_EQ((int)ex.ee, 0);
    kinds[e.kind]++;
    if (e.kind == EN_WILD) {
      CHECK(e.wild.level >= 15 && e.wild.level <= 20);  // BL-5..BL
      bool inPool = false;
      for (int k = 0; k < ROUTE_POOL && ROUTES[R_ROUTE1].pool[k].weight; k++)
        inPool |= wildForm(ROUTES[R_ROUTE1].pool[k].species, e.wild.level) == e.wild.species;
      CHECK(inPool);
      CHECK(dexHas(t->seen, e.wild.species));
    }
  }
  // 70 % salvaje, 20 % entrenador, 10 % objeto
  CHECK(kinds[EN_WILD] > 1950 && kinds[EN_WILD] < 2250);
  CHECK(kinds[EN_TRAINER] > 480 && kinds[EN_TRAINER] < 720);
  CHECK(kinds[EN_ITEM] > 210 && kinds[EN_ITEM] < 390);
  CHECK_EQ(kinds[EN_LEGEND], 0);
}

TEST(explore, eventos_afuera_por_zona) {
  Trainer *t = trainerAt(30);
  Explore ex;
  Rng rng;
  rng.seed(11);
  int kinds[5] = {}, rareNear = 0, rareFar = 0;
  for (int i = 0; i < 4000; i++) {
    uint8_t zone = i % 2 ? ZONE_FAR : ZONE_NEAR;
    Encounter e = exploreOutdoorEvent(ex, *t, rng, zone, T0);
    kinds[e.kind]++;
    if (e.kind != EN_WILD) continue;
    CHECK_EQ((int)e.expiresAt, (int)(T0 + WILD_LINGER_S));
    // el tier se mira en la forma base: busca de quien evoluciona
    uint8_t s = e.wild.species;
    for (bool again = true; again; ) {
      again = false;
      for (int k = 0; k < NUM_EVOS; k++)
        if (EVO_TBL[k].to == s) { s = EVO_TBL[k].from; again = true; }
    }
    if (zone == ZONE_NEAR) {
      CHECK(e.wild.level >= 26 && e.wild.level <= 31);
      rareNear += WILD_TIER[s] == WT_RARE;
    } else {
      CHECK(e.wild.level >= 28 && e.wild.level <= 38);
      rareFar += WILD_TIER[s] == WT_RARE;
    }
  }
  CHECK_EQ(rareNear, 0);
  CHECK(rareFar > 100);
  CHECK(kinds[EN_WILD] > 2250 && kinds[EN_WILD] < 2550);
  CHECK(kinds[EN_TRAINER] > 480 && kinds[EN_TRAINER] < 720);
  CHECK(kinds[EN_ITEM] > 480 && kinds[EN_ITEM] < 720);
  CHECK(kinds[EN_NOTHING] > 280 && kinds[EN_NOTHING] < 520);
}

TEST(explore, zona_lejana_por_gps_o_uno_de_cada_cinco) {
  Explore ex;
  exploreSetHome(ex, 13700000, -89200000);
  CHECK_EQ((int)exploreZone(ex, true, 13701000, -89200000), (int)ZONE_NEAR);  // ~111 m
  CHECK_EQ((int)exploreZone(ex, true, 13720000, -89200000), (int)ZONE_FAR);   // ~2,2 km
  int far = 0;
  for (int i = 0; i < 50; i++) far += exploreZone(ex, false, 0, 0) == ZONE_FAR;
  CHECK_EQ(far, 10);
}

TEST(explore, distancia_geografica) {
  uint32_t d = geoDistanceM(13700000, -89200000, 13701000, -89200000);
  CHECK(d >= 110 && d <= 112);
  d = geoDistanceM(13700000, -89200000, 13700000, -89190000);  // 0,01 grados de longitud
  CHECK(d >= 1075 && d <= 1090);  // 1112 m * cos(13,7)
}

TEST(explore, entrenadores_de_ruta) {
  Rng rng;
  rng.seed(5);
  for (int i = 0; i < 500; i++) {
    RouteTrainer rt;
    uint8_t bl = (uint8_t)(5 + i % 60);
    buildRouteTrainer(rt, bl, rng);
    CHECK(rt.count >= 1 && rt.count <= (bl < 15 ? 2 : 3));
    CHECK(bl >= TRAINER_CLASSES[rt.cls].minBl);
    for (int k = 0; k < rt.count; k++) {
      CHECK(rt.party[k].level + 3 >= bl && rt.party[k].level <= bl + 2);
      if (k) CHECK(rt.party[k].level >= rt.party[k - 1].level);
    }
    uint8_t last = rt.party[rt.count - 1].level;
    CHECK_EQ((int)rt.prize, (int)prizeMoney(TRAINER_CLASSES[rt.cls].prizeBase, last));
    BattleSide side;
    routeTrainerToSide(rt, side);
    CHECK_EQ((int)side.count, (int)rt.count);
    CHECK_EQ((int)side.potions, 0);
  }
}

TEST(explore, legendario_aparece_huye_y_vuelve_en_una_semana) {
  Trainer *t = trainerAt(50);
  t->badges = 0xFF;
  Explore ex;
  Rng rng;
  rng.seed(9);
  Encounter e;
  int tries = 0;
  do {
    ex.ee = EE_PER_SEARCH;
    exploreSearchRoute(ex, *t, rng, R_SEAFOAM, T0, e);
  } while (e.kind != EN_LEGEND && ++tries < 2000);
  CHECK_EQ((int)e.kind, (int)EN_LEGEND);
  CHECK_EQ((int)e.wild.species, ARTICUNO);
  CHECK_EQ((int)e.wild.level, 55);  // Base Level + 5
  exploreLegendResult(ex, e.legend, false, T0);
  for (int i = 0; i < 500; i++) {
    ex.ee = EE_PER_SEARCH;
    exploreSearchRoute(ex, *t, rng, R_SEAFOAM, T0 + 86400, e);
    CHECK(e.kind != EN_LEGEND);
  }
  uint32_t later = T0 + LEGEND_BACK_S;
  tries = 0;
  do {
    ex.ee = EE_PER_SEARCH;
    exploreSearchRoute(ex, *t, rng, R_SEAFOAM, later, e);
  } while (e.kind != EN_LEGEND && ++tries < 2000);
  CHECK_EQ((int)e.kind, (int)EN_LEGEND);
  exploreLegendResult(ex, e.legend, true, later);
  for (int i = 0; i < 500; i++) {
    ex.ee = EE_PER_SEARCH;
    exploreSearchRoute(ex, *t, rng, R_SEAFOAM, later + LEGEND_BACK_S, e);
    CHECK(e.kind != EN_LEGEND);
  }
}

TEST(explore, pokestops_colocar_y_girar) {
  Trainer *t = trainerAt(25);
  Explore ex;
  Rng rng;
  CHECK_EQ((int)stopPlace(ex, 13710000, -89200000), (int)STOP_NO_HOME);
  exploreSetHome(ex, 13700000, -89200000);
  CHECK_EQ((int)stopPlace(ex, 13701000, -89200000), (int)STOP_TOO_CLOSE);  // 111 m de casa
  CHECK_EQ((int)stopPlace(ex, 13703000, -89200000), (int)STOP_OK);         // 333 m
  CHECK_EQ((int)stopPlace(ex, 13704000, -89200000), (int)STOP_TOO_CLOSE);  // 111 m de la otra
  for (int i = 1; i < MAX_STOPS; i++) CHECK_EQ((int)stopPlace(ex, 13703000 + i * 3000, -89200000), (int)STOP_OK);
  CHECK_EQ((int)stopPlace(ex, 13800000, -89200000), (int)STOP_FULL);

  uint32_t d;
  CHECK_EQ((int)stopNearest(ex, 13703100, -89200000, &d), 0);
  CHECK(d < 15);
  Loot loot;
  CHECK_EQ((int)stopSpin(ex, *t, rng, 0, 13704000, -89200000, T0, loot), (int)STOP_TOO_FAR);
  CHECK_EQ((int)stopSpin(ex, *t, rng, 0, 13703200, -89200000, T0, loot), (int)STOP_OK);  // 22 m
  int got = loot.berries + (loot.eggTier ? 1 : 0);
  for (int i = 0; i < loot.count; i++) got += loot.n[i];
  CHECK(got >= 3);
  CHECK_EQ((int)stopSpin(ex, *t, rng, 0, 13703000, -89200000, T0 + 60, loot), (int)STOP_COOLDOWN);
  CHECK_EQ((int)stopSpin(ex, *t, rng, 0, 13703000, -89200000, T0 + STOP_COOLDOWN_S, loot), (int)STOP_OK);
  CHECK_EQ((int)stopRemove(ex, 0), (int)STOP_OK);
  CHECK_EQ((int)ex.stopCount, MAX_STOPS - 1);
}

TEST(explore, botin_sigue_la_franja_del_nivel) {
  Rng rng;
  rng.seed(2);
  Trainer *t = trainerAt(10);
  Explore ex;
  for (int i = 0; i < 300; i++) { Loot l; exploreLoot(ex, *t, rng, l); }
  CHECK(t->bag[IT_POKE_BALL] > 0);
  CHECK_EQ((int)t->bag[IT_GREAT_BALL], 0);
  CHECK_EQ((int)t->bag[IT_ULTRA_BALL], 0);
  t = trainerAt(40);
  for (int i = 0; i < 300; i++) { Loot l; exploreLoot(ex, *t, rng, l); }
  CHECK(t->bag[IT_ULTRA_BALL] > 0);
  CHECK_EQ((int)t->bag[IT_POKE_BALL], 0);
}

TEST(explore, mercader_ambulante) {
  Rng rng;
  rng.seed(4);
  Trainer *t = trainerAt(20);
  Explore ex;
  Loot l;
  int merchants = 0;
  for (int i = 0; i < 2000; i++) {
    exploreLoot(ex, *t, rng, l);
    if (!l.merchant) continue;
    merchants++;
    for (int k = 0; k < MERCHANT_ITEMS; k++) CHECK(l.offer[k].price > 0);
  }
  CHECK(merchants > 140 && merchants < 270);  // 10 %
  do exploreLoot(ex, *t, rng, l); while (!l.merchant);
  t->money = l.offer[0].price;
  uint16_t had = t->bag[l.offer[0].item];
  CHECK(merchantBuy(*t, l, 0));
  CHECK_EQ((int)t->money, 0);
  CHECK_EQ((int)t->bag[l.offer[0].item], (int)had + 1);
  CHECK(!merchantBuy(*t, l, 1));  // sin dinero
}

TEST(explore, incienso_seis_salvajes_en_media_hora) {
  Trainer *t = trainerAt(15);
  Explore ex;
  Rng rng;
  Encounter e;
  CHECK(!incenseStart(ex, *t, R_ROUTE1, T0));  // sin incienso
  addItem(*t, IT_INCENSE);
  CHECK(!incenseStart(ex, *t, R_MTMOON, T0));  // ruta cerrada
  CHECK(incenseStart(ex, *t, R_ROUTE1, T0));
  CHECK_EQ((int)t->bag[IT_INCENSE], 0);
  int spawns = 0;
  for (uint32_t s = 0; s < INCENSE_S + 600; s += 30)
    if (incenseTick(ex, *t, rng, T0 + s, e)) {
      spawns++;
      CHECK_EQ((int)e.kind, (int)EN_WILD);
      CHECK(e.wild.level >= 10 && e.wild.level <= 15);
    }
  CHECK_EQ(spawns, 6);
  CHECK(!incenseActive(ex, T0 + INCENSE_S));
}

TEST(explore, objetos_encontrados_y_tope_de_caramelos) {
  Rng rng;
  rng.seed(8);
  Trainer *t = trainerAt(60);
  Explore ex;
  int candies = 0, nuggets = 0;
  for (int i = 0; i < 3000; i++) {
    uint8_t item, n;
    findItem(ex, *t, rng, 60, true, T0, item, n);
    candies += item == IT_RARE_CANDY;
    nuggets += item == IT_NUGGET;
    CHECK(item == ITEM_BERRY || item < IT_COUNT);
  }
  CHECK_EQ(candies, 1);  // uno al dia como mucho
  CHECK(nuggets > 30 && nuggets < 95);  // 2 % afuera
}

TEST(explore, guardar_y_cargar) {
  Explore ex;
  Rng rng;
  ex.ee = 1234;
  ex.meters = 56789;
  eggAdd(ex, rng, EGG_RARE);
  exploreSetHome(ex, 13700000, -89200000);
  stopPlace(ex, 13703000, -89200000);
  static uint8_t buf[4096];
  size_t n = exploreSave(ex, buf, sizeof(buf));
  CHECK_EQ((int)n, (int)exploreSaveSize());
  Explore back;
  CHECK(exploreLoad(back, buf, n));
  CHECK_EQ((int)back.ee, 1234);
  CHECK_EQ((int)back.meters, 56789);
  CHECK_EQ((int)back.eggs[0].species, (int)ex.eggs[0].species);
  CHECK_EQ((int)back.stopCount, 1);
  buf[SAVE_HEADER + 3] ^= 0x40;
  CHECK(!exploreLoad(back, buf, n));
  CHECK_EQ(0, memcmp(buf, "PTE1", 4));
}
