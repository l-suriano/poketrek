#pragma once
#include <stdint.h>
#include "battle.h"
#include "trainer.h"
#include "explore.h"
#include "rewards.h"

// Torneos de PokeTrek (reglas: "Tournaments"): siempre abiertos, con cuota,
// 8 entrenadores a eliminacion directa (3 victorias para ser campeon), 3 vs 3
// con Pokemon del equipo o del PC, curados entre rondas y 2 objetos por combate.
// Tambien el Daily Challenge (mismo cuadro para todos ese dia) y la Featured
// Cup de la semana.

enum Cup : uint8_t { CUP_OPEN, CUP_LITTLE, CUP_L30, CUP_FLAT50, CUP_MONO, CUP_NO_ITEMS, CUP_RANDOM,
                     CUP_CHAMPION, NUM_CUPS };
enum LevelRule : uint8_t { LV_SCALED, LV_FIXED, LV_CAP };
struct CupInfo {
  const char *en, *es;
  const char *city;
  uint8_t levelRule, level;  // nivel fijo o tope
};
extern const CupInfo CUPS[NUM_CUPS];
const char *cupName(uint8_t cup);

enum TourTier : uint8_t { TT_ROOKIE, TT_GREAT, TT_ULTRA, TT_MASTER, NUM_TOUR_TIERS };
struct TierInfo {
  const char *en, *es;
  int8_t offset;     // niveles sobre el Base Level de tus 3 Pokemon
  int8_t aiDelta;    // IA: -1 / 0 / +1 sobre la normal; 9 = siempre 4
  uint16_t fee;
  uint8_t champBp, finBp;
};
extern const TierInfo TOUR_TIERS[NUM_TOUR_TIERS];

#define TOUR_TEAM 3
#define TOUR_ROUNDS 3
#define TOUR_ITEMS 2           // objetos por combate (0 en la No Items Cup)
#define FEATURED_DISCOUNT 25   // % de descuento en la Featured Cup
#define DAILY_LEVEL 50

// ---- Featured Cup (cambia el lunes a las 00:00) ----
struct Featured { uint8_t cup; uint8_t type; };  // type = T_NONE salvo en Mono-Type
Featured featuredCup(uint16_t week);

// ---- Daily Challenge ----
enum DailyRule : uint8_t { DR_OPEN, DR_MONO, DR_NO_ITEMS, DR_LITTLE, NUM_DAILY_RULES };
enum PrizeKind : uint8_t { PZ_NONE, PZ_ITEM, PZ_TM, PZ_RIBBON };
struct DailyChallenge {
  uint8_t rule, type;        // type para DR_MONO
  uint8_t prizeKind, prizeId;  // Daily Prize del campeon
};
DailyChallenge dailyChallenge(uint16_t day);

// ---- una inscripcion ----
struct TourPick { uint8_t inBox; uint8_t idx; };  // inBox 0 = equipo, 1 = PC

enum TourSpecial : uint8_t { SPC_NONE, SPC_BLUE, SPC_GYM, SPC_ELITE };

struct Tournament {
  uint8_t cup = CUP_OPEN, tier = TT_GREAT;
  bool daily = false, featured = false;
  uint8_t monoType = T_NONE;   // Mono-Type: el tipo que comparten
  bool noItems = false, little = false;
  uint8_t oppLevel = 50;       // nivel de los rivales
  uint32_t seed = 0;           // cuadro de rivales
  TourPick picks[TOUR_TEAM];
  uint32_t fee = 0;
  uint8_t wins = 0;
  bool finished = false;
  bool anyFainted = false;
  uint8_t special = SPC_NONE, specialIdx = 0;  // retador especial en la final
  int32_t score = 0;           // Daily: 1000 por victoria + 5 por % de PS - 10 por turno
};

enum TourResult : uint8_t { TOUR_OK, TOUR_BAD_PICK, TOUR_DUPLICATE, TOUR_RULE, TOUR_NO_MONEY,
                            TOUR_LOCKED, TOUR_DAILY_DONE, TOUR_NOT_ENOUGH };

uint32_t tourFee(uint8_t tier, bool featured);
// comprueba si los 3 elegidos cumplen la regla de la copa
TourResult tourCheckTeam(const Trainer &t, const TourPick picks[TOUR_TEAM], bool little, uint8_t monoType);
// se inscribe y paga la cuota. En la Random Cup los 3 salen al azar del PC.
TourResult tourEnter(Tournament &run, Trainer &t, Progress &p, uint8_t cup, uint8_t tier,
                     const TourPick picks[TOUR_TEAM], uint32_t now, Rng &rng);
// Daily Challenge: gratis, un intento al dia; cuenta el dia al entrar
TourResult dailyEnter(Tournament &run, Trainer &t, Progress &p, Explore &ex,
                      const TourPick picks[TOUR_TEAM], uint32_t now, Rng &rng);

// tu equipo para el combate: copias curadas al nivel de la copa (el equipo real no cambia)
void tourPlayerSide(const Tournament &run, const Trainer &t, BattleSide &side);
uint8_t tourItemsAllowed(const Tournament &run);

// rival de la ronda actual (0..2); en la final puede ser un retador especial
struct TourRival { uint8_t special; uint8_t idx; };  // idx: clase de entrenador, gimnasio o ELITE
TourRival tourRival(const Tournament &run);
void tourOpponent(const Tournament &run, const BattleSide &player, BattleSide &opp);

// resultado del combate de esta ronda (hpPct = % de PS que le quedan a tu equipo)
void tourReport(Tournament &run, bool won, bool anyFainted, uint8_t hpPct, uint16_t turns);
bool tourOver(const Tournament &run);

struct TourPayout {
  uint32_t money = 0;
  uint16_t bp = 0;
  bool champion = false, finalist = false, perfect = false, specialBeaten = false;
  uint8_t prizeKind = PZ_NONE, prizeId = 0;    // Daily Prize
  uint8_t weeklyKind = PZ_NONE, weeklyId = 0;  // premio semanal de la Featured Cup
};
// reparte premios al terminar: dinero, BP, confianza, Featured Cup y Daily Prize
TourPayout tourFinish(Tournament &run, Trainer &t, Progress &p, Explore &ex, uint32_t now);

// XP en torneos: x1,5 en Daily y Featured; en copas de nivel fijo o tope la XP
// usa el nivel real de tu Pokemon como nivel del rival (no se farmea XP de nivel 50)
ExpBonus tourExpBonus(const Tournament &run);
uint8_t tourExpFoeLevel(const Tournament &run, uint8_t ownRealLevel, uint8_t foeLevel);

bool canStillEvolve(uint8_t species);
bool hasType(uint8_t species, uint8_t type);
