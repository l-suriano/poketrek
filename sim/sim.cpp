// Simulador de PokeTrek: miles de peleas contra cada gimnasio para afinar la
// dificultad (reglas: "Tuning, data & licensing"). Corre en el PC con el mismo
// motor que la placa.
//
//   ./sim [N]         tabla de % de victorias con los valores de gyms.cpp
//   ./sim tune [N]    busca para cada lider el levelPct que deja al jugador
//                     casual en su objetivo (casualTarget) y lo imprime
//   ./sim tune-elite [N]  lo mismo para la Liga (ELITE_PCT, LEAGUE_TARGET)
//   ./sim tour [N]    torneos de la Open Cup por categoria: % de victorias por
//                     combate, % de campeones y ganancia media por inscripcion
//
// Dos perfiles de jugador:
//   casual    6 Pokemon al azar, movimientos por defecto, IA nivel 2, 2 pociones
//   preparado al menos 3 con ventaja de tipo contra el lider, movimientos con
//             cobertura (incluye MT), IA nivel 4, 5 pociones
#include "../battle.h"
#include "../gyms.h"
#include "../tournament.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t LEVELS[] = { 10, 15, 20, 25, 32, 40, 45, 52, 60 };
#define NUM_LEVELS (int)(sizeof(LEVELS) / sizeof(LEVELS[0]))
// objetivo del jugador CASUAL: rampa de 50 % (Brock) a 15 % (Giovanni)
static double casualTarget(int gym) { return 50.0 - 5.0 * gym; }

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

#define LEAGUE 0xFF  // "gym" especial: equipo para la Liga (sin un tipo al que apuntar)

static void buildPlayer(BattleSide &p, uint8_t level, bool prepared, uint8_t gym, Rng &rng) {
  bool league = gym == LEAGUE;
  p = BattleSide();
  p.count = 6;
  uint8_t used[6] = {0};
  for (int i = 0; i < 6; i++) {
    uint8_t sp;
    bool dup;
    do {
      sp = randomBase(rng);
      // el preparado lleva al menos 3 con ventaja de tipo (miramos la forma evolucionada)
      if (prepared && !league && i < 3 && !hasAdvantage(evolveForLevel(sp, level), GYMS[gym].type)) {
        dup = true;
        continue;
      }
      dup = false;
      for (int j = 0; j < i; j++) dup |= used[j] == sp;
    } while (dup);
    used[i] = sp;
    makeMon(p.party[i], sp, level, rng);
  }
  if (prepared) {
    // cobertura contra el tipo del lider, o contra los cinco tipos de la Liga
    BattleMon dummies[NUM_ELITE];
    uint8_t nd = league ? NUM_ELITE : 1;
    for (int d = 0; d < nd; d++) {
      monInit(dummies[d], 1, level);
      dummies[d].type1 = league ? ELITE[d].type : GYMS[gym].type;
      dummies[d].type2 = T_NONE;
    }
    for (int i = 0; i < 6; i++) pickMovesSmart(p.party[i], dummies, nd, true);
  }
  p.aiTier = prepared ? 4 : 2;
  // para la Liga se va con la mochila llena
  p.potions = league ? (prepared ? 10 : 5) : (prepared ? 5 : 2);
  p.potionHeal = potionForLevel(level);
}

// devuelve true si gana el jugador
static bool fight(uint8_t gym, uint8_t level, bool prepared, uint32_t seed, int8_t pct) {
  Rng rng;
  rng.seed(seed * 2654435761u + gym * 97 + level);
  Battle b;
  b = Battle();
  buildPlayer(b.side[0], level, prepared, gym, rng);
  buildGymTeam(b.side[1], gym, b.side[0], rng, pct);
  battleInit(b, rng.next(), false);
  for (int turn = 0; turn < 1500 && !b.outcome; turn++) {
    for (int s = 0; s < 2; s++)
      if (battleNeedsSwitch(b, s)) battleSwitchIn(b, s, battleAiPickSwitch(b, s));
    battleTurn(b, battleAiChoose(b, 0), battleAiChoose(b, 1));
  }
  return b.outcome == OUT_WIN0;
}

// Entre combates de la Liga solo valen objetos (reglas): Revivir a los
// debilitados (vuelven con media vida), Cura Total al que tenga estado y
// pociones al que este por debajo de la mitad.
struct LeagueBag { uint8_t revives, fullHeals; };

static void healBetween(BattleSide &me, LeagueBag &bag) {
  for (int i = 0; i < me.count; i++) {
    BattleMon &m = me.party[i];
    if (!m.hp && bag.revives) { bag.revives--; m.hp = m.maxHp / 2; m.status = STS_NONE; }
    if (m.hp && m.status && bag.fullHeals) { bag.fullHeals--; m.status = STS_NONE; m.sleepTurns = 0; }
  }
  for (int i = 0; i < me.count; i++) {
    BattleMon &m = me.party[i];
    while (m.hp && m.hp * 2 < m.maxHp && me.potions) {
      me.potions--;
      uint32_t h = m.hp + me.potionHeal;
      m.hp = (uint16_t)(h > m.maxHp ? m.maxHp : h);
    }
  }
}

// la Liga entera seguida: el equipo conserva PS, PP, estados y objetos entre
// combates. Devuelve cuantos rivales gano (5 = Campeon).
static int leagueRun(uint8_t level, bool prepared, uint32_t seed, int8_t pct) {
  Rng rng;
  rng.seed(seed * 2246822519u + level);
  BattleSide me;
  buildPlayer(me, level, prepared, LEAGUE, rng);
  LeagueBag bag = { (uint8_t)(prepared ? 5 : 2), (uint8_t)(prepared ? 3 : 1) };
  for (int idx = 0; idx < NUM_ELITE; idx++) {
    if (idx) healBetween(me, bag);
    Battle b;
    b = Battle();
    b.side[0] = me;
    buildEliteTeam(b.side[1], (uint8_t)idx, me, rng, pct);
    battleInit(b, rng.next(), false);
    for (int turn = 0; turn < 1500 && !b.outcome; turn++) {
      for (int s = 0; s < 2; s++)
        if (battleNeedsSwitch(b, s)) battleSwitchIn(b, s, battleAiPickSwitch(b, s));
      battleTurn(b, battleAiChoose(b, 0), battleAiChoose(b, 1));
    }
    if (b.outcome != OUT_WIN0) return idx;
    battleEnd(b);
    me = b.side[0];
  }
  return NUM_ELITE;
}

// % de corridas que llegan a ganar al rival `reach` (5 = Campeon)
static double leagueRate(uint8_t level, bool prepared, int n, int8_t pct, int reach = NUM_ELITE) {
  int ok = 0;
  for (int i = 0; i < n; i++) ok += leagueRun(level, prepared, (uint32_t)i + 1, pct) >= reach;
  return 100.0 * ok / n;
}

static double leagueBandRate(uint8_t band, bool prepared, int n, int8_t pct) {
  double sum = 0;
  int k = 0;
  for (int l = 0; l < NUM_LEVELS; l++) {
    if (gymBand(LEVELS[l]) != band) continue;
    sum += leagueRate(LEVELS[l], prepared, n, pct);
    k++;
  }
  return k ? sum / k : 0;
}

static void printLeague(int n, const int8_t *pct) {
  printf("== LIGA: Alto Mando + Campeon seguidos (+%d/%d/%d %%, Campeon +%d %% extra) ==\n",
         pct[0], pct[1], pct[2], CHAMPION_EXTRA_PCT);
  printf("%-26s", "");
  for (int l = 0; l < NUM_LEVELS; l++) printf("  Nv%-4d", LEVELS[l]);
  printf("\n");
  for (int prof = 0; prof < 2; prof++) {
    bool prepared = prof == 1;
    for (int reach = 4; reach <= NUM_ELITE; reach++) {
      printf("%-10s %-15s", prepared ? "preparado" : "casual", reach == 4 ? "llega a Blue" : "es Campeon");
      for (int l = 0; l < NUM_LEVELS; l++)
        printf("  %4.0f%% ", leagueRate(LEVELS[l], prepared, n, pct[gymBand(LEVELS[l])], reach));
      printf("\n");
      fflush(stdout);
    }
  }
  printf("\n");
}

// mismo metodo que tune(), pero la Liga es el final del juego: la referencia es
// el jugador PREPARADO (con el casual, el Alto Mando tendria que ir ~40 % por
// debajo de tu nivel). Un % por tramo para ser Campeon LEAGUE_TARGET % de las veces.
#define LEAGUE_TARGET 50.0
static void tuneLeague(int n) {
  int8_t best[3];
  printf("Buscando el +%% de nivel de la Liga para que el PREPARADO sea Campeon el %.0f %% de las veces...\n",
         LEAGUE_TARGET);
  for (uint8_t band = 0; band < 3; band++) {
    int lo = -60, hi = 120;
    while (hi - lo > 1) {
      int mid = (lo + hi) / 2;
      if (leagueBandRate(band, true, n, (int8_t)mid) > LEAGUE_TARGET) lo = mid;
      else hi = mid;
    }
    double wl = leagueBandRate(band, true, n, (int8_t)lo), wh = leagueBandRate(band, true, n, (int8_t)hi);
    best[band] = (int8_t)((wl - LEAGUE_TARGET) < (LEAGUE_TARGET - wh) ? lo : hi);
  }
  printf("  ELITE_PCT = { %d, %d, %d }\n\n", best[0], best[1], best[2]);
  printLeague(n, best);
}

static double winRate(uint8_t gym, uint8_t level, bool prepared, int n, int8_t pct) {
  int wins = 0;
  for (int i = 0; i < n; i++) wins += fight(gym, level, prepared, (uint32_t)i + 1, pct);
  return 100.0 * wins / n;
}

// media sobre los niveles de un tramo de Base Level (los de LEVELS que caen en el)
static double bandWinRate(uint8_t gym, uint8_t band, bool prepared, int n, int8_t pct) {
  double sum = 0;
  int k = 0;
  for (int l = 0; l < NUM_LEVELS; l++) {
    if (gymBand(LEVELS[l]) != band) continue;
    sum += winRate(gym, LEVELS[l], prepared, n, pct);
    k++;
  }
  return k ? sum / k : 0;
}

static void printTable(int n, const int8_t (*pct)[3]) {
  for (int prof = 1; prof >= 0; prof--) {
    bool prepared = prof == 1;
    printf("== jugador %s ==\n", prepared ? "PREPARADO (cobertura, IA 4, 5 pociones)" : "CASUAL (al azar, IA 2, 2 pociones)");
    printf("%-10s %-12s", "Lider", "+% por tramo");
    for (int l = 0; l < NUM_LEVELS; l++) printf("  Nv%-4d", LEVELS[l]);
    printf("  media\n");
    for (int g = 0; g < NUM_GYMS; g++) {
      const int8_t *p = pct ? pct[g] : GYMS[g].levelPct;
      printf("%-10s %3d/%3d/%3d ", GYMS[g].leader, p[0], p[1], p[2]);
      double sum = 0;
      for (int l = 0; l < NUM_LEVELS; l++) {
        double w = winRate((uint8_t)g, LEVELS[l], prepared, n, p[gymBand(LEVELS[l])]);
        sum += w;
        printf("  %4.0f%% ", w);
      }
      printf("  %4.0f%%\n", sum / NUM_LEVELS);
      fflush(stdout);
    }
    printf("\n");
  }
}

// el % de victorias baja al subir el nivel del lider: busqueda binaria por
// lider y por tramo; al final imprime las lineas para pegar en gyms.cpp
static void tune(int n) {
  int8_t best[NUM_GYMS][3];
  printf("Buscando el +%% de nivel que deja al jugador CASUAL en su objetivo en cada tramo...\n");
  for (int g = 0; g < NUM_GYMS; g++) {
    double target = casualTarget(g);
    for (uint8_t band = 0; band < 3; band++) {
      int lo = -60, hi = 120;
      while (hi - lo > 1) {
        int mid = (lo + hi) / 2;
        if (bandWinRate((uint8_t)g, band, false, n, (int8_t)mid) > target) lo = mid;
        else hi = mid;
      }
      double wl = bandWinRate((uint8_t)g, band, false, n, (int8_t)lo);
      double wh = bandWinRate((uint8_t)g, band, false, n, (int8_t)hi);
      best[g][band] = (int8_t)((wl - target) < (target - wh) ? lo : hi);
    }
    printf("  %-10s objetivo %2.0f%%  { %3d, %3d, %3d }\n", GYMS[g].leader, target,
           best[g][0], best[g][1], best[g][2]);
    fflush(stdout);
  }
  printf("\n");
  printTable(n, best);
}

// ---------------------------------------------------------------- torneos --
// Tres perfiles con 3 Pokemon al azar (evolucionados para su nivel):
//   casual    movimientos por defecto, IA 2
//   medio     movimientos por defecto, IA 3
//   preparado cobertura con MT, IA 4
// Las reglas piden que el medio quede mas o menos a la par y el preparado gane.
enum { PR_CASUAL, PR_AVERAGE, PR_PREPARED, NUM_PROFILES };
static const char *PROFILE_NAME[NUM_PROFILES] = { "casual", "medio", "preparado" };

static void buildTourPlayer(BattleSide &p, uint8_t level, int profile, Rng &rng) {
  p = BattleSide();
  p.count = TOUR_TEAM;
  uint8_t used[TOUR_TEAM] = {0};
  for (int i = 0; i < TOUR_TEAM; i++) {
    uint8_t sp;
    bool dup;
    do {
      sp = randomBase(rng);
      dup = false;
      for (int j = 0; j < i; j++) dup |= used[j] == sp;
    } while (dup);
    used[i] = sp;
    makeMon(p.party[i], sp, level, rng);
  }
  if (profile == PR_PREPARED) {
    // cobertura general: contra un rival de cada tipo frecuente
    static const uint8_t COMMON[5] = { T_NORMAL, T_WATER, T_GRASS, T_FIRE, T_POISON };
    BattleMon dummies[5];
    for (int d = 0; d < 5; d++) {
      monInit(dummies[d], 1, level);
      dummies[d].type1 = COMMON[d];
      dummies[d].type2 = T_NONE;
    }
    for (int i = 0; i < TOUR_TEAM; i++) pickMovesSmart(p.party[i], dummies, 5, true);
  }
  p.aiTier = profile == PR_PREPARED ? 4 : profile == PR_AVERAGE ? 3 : 2;
  p.potions = TOUR_ITEMS;
  p.potionHeal = potionForLevel(level);
}

// un torneo entero: devuelve las victorias (0..3)
static int tourRun(uint8_t tier, uint8_t level, int profile, uint32_t seed) {
  Rng rng;
  rng.seed(seed * 2654435761u + tier * 131 + level * 7 + profile);
  Tournament run;
  run.cup = CUP_OPEN;
  run.tier = tier;
  run.oppLevel = (uint8_t)(level + TOUR_TIERS[tier].offset);
  run.seed = rng.next();
  uint32_t teamSeed = rng.next();
  while (!tourOver(run)) {
    Rng teamRng;
    teamRng.seed(teamSeed);  // el mismo equipo, curado, en cada ronda
    Battle b;
    buildTourPlayer(b.side[0], level, profile, teamRng);
    tourOpponent(run, b.side[0], b.side[1]);
    battleInit(b, rng.next(), false);
    for (int turn = 0; turn < 1500 && !b.outcome; turn++) {
      for (int s = 0; s < 2; s++)
        if (battleNeedsSwitch(b, s)) battleSwitchIn(b, s, battleAiPickSwitch(b, s));
      battleTurn(b, battleAiChoose(b, 0), battleAiChoose(b, 1));
    }
    tourReport(run, b.outcome == OUT_WIN0, false, 0, 0);
  }
  return run.wins;
}

static void printTours(int n) {
  printf("Torneos (Open Cup): %% victorias por combate / %% campeon / ganancia media por inscripcion\n");
  printf("(ganancia en cuotas: 0 = a la par; ronda 1 pierde la cuota, semifinal la devuelve,\n");
  printf(" final x1,5, campeon x3)\n\n");
  for (int tier = 0; tier < NUM_TOUR_TIERS; tier++) {
    printf("%-7s", TOUR_TIERS[tier].en);
    for (int pr = 0; pr < NUM_PROFILES; pr++) printf(" | %-24s", PROFILE_NAME[pr]);
    printf("\n");
    double sum[NUM_PROFILES] = {};
    int rows = 0;
    for (int li = 2; li < NUM_LEVELS; li++, rows++) {
      uint8_t level = LEVELS[li];
      printf("  L%-3d ", level);
      for (int pr = 0; pr < NUM_PROFILES; pr++) {
        int battles = 0, won = 0, champs = 0;
        double net = 0;
        for (int i = 0; i < n; i++) {
          int w = tourRun((uint8_t)tier, level, pr, (uint32_t)i);
          won += w;
          battles += w + (w < TOUR_ROUNDS ? 1 : 0);
          champs += w == TOUR_ROUNDS;
          net += (w == 3 ? 3.0 : w == 2 ? 1.5 : w == 1 ? 1.0 : 0.0) - 1.0;
        }
        sum[pr] += net / n;
        printf(" | %3.0f%% %3.0f%% %+5.2f cuotas  ", 100.0 * won / battles, 100.0 * champs / n, net / n);
      }
      printf("\n");
    }
    printf("  media");
    for (int pr = 0; pr < NUM_PROFILES; pr++) printf(" |           %+5.2f cuotas  ", sum[pr] / rows);
    printf("\n\n");
  }
}

int main(int argc, char **argv) {
  bool tours = argc > 1 && strcmp(argv[1], "tour") == 0;
  if (tours) {
    int n = argc > 2 ? atoi(argv[2]) : 300;
    printTours(n < 1 ? 1 : n);
    return 0;
  }
  bool tuning = argc > 1 && strcmp(argv[1], "tune") == 0;
  bool tuningLeague = argc > 1 && strcmp(argv[1], "tune-elite") == 0;
  bool league = argc > 2 && strcmp(argv[1], "league") == 0;  // ./sim league PCT [N]
  int argN = (tuning || tuningLeague) ? 2 : league ? 3 : 1;
  int n = argc > argN ? atoi(argv[argN]) : 300;
  if (n < 1) n = 1;
  printf("PokeTrek - %% de victorias del jugador (%d batallas o corridas por casilla)\n", n);
  printf("Jugador con los 6 Pokemon al nivel indicado (Base Level = ese nivel).\n");
  printf("Objetivo: casual de 50 %% (Brock) a 15 %% (Giovanni); Campeon el %.0f %% de las veces.\n\n",
         LEAGUE_TARGET);
  if (tuning) tune(n);
  else if (tuningLeague) tuneLeague(n);
  else if (league) {
    int8_t p = (int8_t)atoi(argv[2]);
    int8_t pct[3] = { p, p, p };
    printLeague(n, pct);
  }
  else {
    printTable(n, nullptr);
    printLeague(n, ELITE_PCT);
  }
  return 0;
}
