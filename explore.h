#pragma once
#include <stdint.h>
#include <stddef.h>
#include "battle.h"
#include "trainer.h"

// Exploracion de PokeTrek (reglas: "Exploring: outside and at home" y
// "Fair play & anti-cheat"): pasos y Explore Energy, eventos al caminar afuera,
// rutas de Kanto desde casa, PokeStops y Supply Drops, huevos por km, incienso,
// el Mercader Ambulante y los legendarios de las rutas. C++ puro: la placa le
// pasa pasos, metros, posicion GPS y la hora; aqui no se lee ningun sensor.

// ---- numeros de las reglas (afinar jugando) ----
#define EE_MAX 5000              // tope del banco de Explore Energy
#define EE_PER_SEARCH 300        // una busqueda en una ruta desde casa
#define STEPS_PER_EVENT 400      // pasos validos afuera por evento
#define STEPS_PER_DROP 1000      // Supply Drop de la placa sin GPS
#define STEP_LEN_MM 700          // km por pasos en la placa sin GPS
#define MAX_SPEED_KMH 15         // por encima no cuenta nada (coche, bus)
#define NEAR_ZONE_M 2000         // zona cercana: menos de 2 km de casa
#define WILD_LINGER_S 600        // un salvaje ignorado se va a los 10 min
#define MAX_STOPS 10
#define STOP_MIN_GAP_M 200       // entre PokeStops y desde casa
#define STOP_SPIN_M 40           // radio para girar una PokeStop
#define STOP_COOLDOWN_S 300
#define MAX_EGGS 3
#define INCENSE_S 1800           // 30 min
#define INCENSE_EVERY_S 300      // un salvaje cada 5 min
#define LEGEND_BACK_S (7UL * 86400UL)  // un legendario huido vuelve en una semana
#define LEGEND_PCT 5             // por busqueda en su ruta (afinar jugando)

// ---- huevos y rareza de los salvajes ----
enum WildTier : uint8_t { WT_NONE = 0, WT_COMMON, WT_MID, WT_RARE };
// rareza de cada especie como salvaje y como huevo (solo formas base; las
// evoluciones salen al subir de nivel). Los legendarios son WT_NONE.
extern const uint8_t WILD_TIER[152];

enum EggTier : uint8_t { EGG_COMMON = 1, EGG_RARE, EGG_VERY_RARE };  // = WildTier
uint16_t eggKm(uint8_t tier);  // 2, 5, 10

struct Egg {
  uint8_t species = 0;  // 0 = hueco vacio
  uint8_t tier = 0;
  bool shiny = false;
  uint16_t walkedM = 0;
};

// ---- rutas desde casa ----
#define ROUTE_POOL 12
#define ROUTE_CHAMPION 9  // badges = 9: hace falta ser Campeon
struct RouteSlot { uint8_t species, weight; };
struct RouteInfo {
  const char *en, *es;
  uint8_t badges;       // medallas para abrirla (ROUTE_CHAMPION = tras la Liga)
  uint8_t legend;       // especie legendaria de la ruta (0 = ninguna)
  uint8_t legendPlus;   // su nivel: Base Level + esto
  RouteSlot pool[ROUTE_POOL];  // formas base comunes y medias (peso 0 = fin)
};
#define NUM_ROUTES 9
extern const RouteInfo ROUTES[NUM_ROUTES];
const char *routeName(uint8_t route);
bool routeOpen(const Trainer &t, uint8_t route);

// ---- entrenadores de ruta ----
struct TrainerClass {
  const char *en, *es;
  uint8_t prizeBase;   // dinero = base * nivel del ultimo (gen 1)
  uint8_t minBl;       // aparece desde este Base Level
  uint8_t pool[6];     // formas base (0 = fin)
};
#define NUM_TRAINER_CLASSES 16
extern const TrainerClass TRAINER_CLASSES[NUM_TRAINER_CLASSES];

#define ROUTE_TRAINER_MAX 3
struct RouteTrainer {
  uint8_t cls = 0;
  uint8_t count = 0;
  BattleMon party[ROUTE_TRAINER_MAX];
  uint8_t aiTier = 1;
  uint32_t prize = 0;
};
// copia el equipo a un lado de la batalla
void routeTrainerToSide(const RouteTrainer &rt, BattleSide &side);

// ---- estado guardado ----
struct PokeStop {
  int32_t latE6 = 0, lonE6 = 0;  // microgrados
  uint32_t readyAt = 0;          // fin del enfriamiento (segundos)
};

#define NUM_ROUTE_LEGENDS 4  // Articuno, Zapdos, Moltres, Mewtwo

struct Explore {
  uint16_t ee = 0;
  uint32_t steps = 0;           // pasos validos totales
  uint32_t meters = 0;          // distancia total (GPS o pasos * 0,7 m)
  uint16_t mmCarry = 0;         // milimetros sueltos de los pasos sin GPS
  uint32_t kmPaid = 0;          // km ya recompensados (confianza, Caramelo Raro)
  uint16_t stepsToEvent = STEPS_PER_EVENT;
  uint16_t stepsToDrop = STEPS_PER_DROP;
  Egg eggs[MAX_EGGS];
  uint32_t incenseUntil = 0, incenseNext = 0;
  uint8_t incenseRoute = 0;
  bool homeSet = false;
  int32_t homeLatE6 = 0, homeLonE6 = 0;
  PokeStop stops[MAX_STOPS];
  uint8_t stopCount = 0;
  uint8_t legendCaught = 0;                 // bit i = legendario i ya atrapado
  uint32_t legendBackAt[NUM_ROUTE_LEGENDS] = {};
  uint16_t candyDay = 0xFFFF;               // dia del ultimo Caramelo Raro gratis
  uint8_t outdoorEvents = 0;                // para la zona lejana sin GPS (1 de 5)
};

void exploreInit(Explore &ex);
uint8_t partyBaseLevel(const Trainer &t);
uint16_t dayIndex(uint32_t now);  // dias desde 1970 (la placa ya da la hora local)

// ---- caminar ----
struct WalkInput {
  uint16_t steps = 0;    // pasos validos (ritmo de caminar ya filtrado)
  uint32_t meters = 0;   // GPS; sin GPS se calcula con los pasos
  bool outside = false;  // lejos del Wi-Fi de casa
  bool gps = false;      // placa -G
  uint8_t speedKmh = 0;
  uint32_t now = 0;
};
struct WalkResult {
  uint16_t ee = 0;            // EE sumada al banco
  uint8_t events = 0;         // eventos afuera que tocan (exploreOutdoorEvent)
  uint8_t drops = 0;          // Supply Drops que tocan (exploreLoot)
  uint16_t km = 0;            // km completos nuevos
  uint8_t eggsReady = 0;      // huevos listos para eclosionar
  uint8_t candies = 0;        // Caramelos Raros por cada 10 km
};
WalkResult exploreWalk(Explore &ex, Trainer &t, const WalkInput &in);

// ---- encuentros ----
enum EncounterKind : uint8_t { EN_NOTHING, EN_WILD, EN_TRAINER, EN_ITEM, EN_LEGEND };
enum Zone : uint8_t { ZONE_HOME, ZONE_NEAR, ZONE_FAR };

struct Encounter {
  uint8_t kind = EN_NOTHING;
  uint8_t zone = ZONE_HOME;
  BattleMon wild;            // EN_WILD y EN_LEGEND
  bool shiny = false;
  uint8_t legend = 0;        // indice 0..3 para exploreLegendResult
  uint32_t expiresAt = 0;    // el salvaje se va si no lo atiendes
  RouteTrainer trainer;      // EN_TRAINER
  uint8_t item = 0, itemCount = 0;  // EN_ITEM (ya en la mochila; ITEM_BERRY = bayas)
};

// distancia en metros entre dos puntos (microgrados)
uint32_t geoDistanceM(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2);
// zona del jugador: con GPS por distancia a casa; sin GPS, lejana 1 de cada 5
uint8_t exploreZone(Explore &ex, bool gps, int32_t latE6, int32_t lonE6);

// evento afuera (60 % salvaje, 15 % entrenador, 15 % objeto, 10 % nada)
Encounter exploreOutdoorEvent(Explore &ex, Trainer &t, Rng &rng, uint8_t zone, uint32_t now,
                              uint16_t shinyDen = 256);
// busqueda en una ruta desde casa: gasta 300 EE (70 % salvaje, 20 % entrenador,
// 10 % objeto). false si la ruta esta cerrada o no hay EE.
bool exploreSearchRoute(Explore &ex, Trainer &t, Rng &rng, uint8_t route, uint32_t now,
                        Encounter &out, uint16_t shinyDen = 256);
// tras luchar contra un legendario: si no se atrapo, vuelve en una semana
void exploreLegendResult(Explore &ex, uint8_t legend, bool caught, uint32_t now);

// salvaje de una zona o ruta al nivel que toca (lo usan los encuentros y el incienso)
void exploreWild(BattleMon &m, uint8_t species, uint8_t level, Rng &rng);
// forma que tiene `species` a `level` solo por evoluciones de nivel (salvajes)
uint8_t wildForm(uint8_t species, uint8_t level);
void buildRouteTrainer(RouteTrainer &rt, uint8_t bl, Rng &rng);

// ---- incienso ----
bool incenseStart(Explore &ex, Trainer &t, uint8_t route, uint32_t now);  // gasta 1
bool incenseActive(const Explore &ex, uint32_t now);
// true si toca un salvaje ahora (uno cada 5 min durante 30 min, sin gastar EE)
bool incenseTick(Explore &ex, Trainer &t, Rng &rng, uint32_t now, Encounter &out,
                 uint16_t shinyDen = 256);

// ---- PokeStops, Supply Drops y Mercader ----
enum StopResult : uint8_t { STOP_OK, STOP_NO_HOME, STOP_FULL, STOP_TOO_CLOSE, STOP_TOO_FAR,
                            STOP_COOLDOWN, STOP_BAD_INDEX };
void exploreSetHome(Explore &ex, int32_t latE6, int32_t lonE6);
StopResult stopPlace(Explore &ex, int32_t latE6, int32_t lonE6);
StopResult stopRemove(Explore &ex, uint8_t idx);
// la PokeStop mas cercana y su distancia (0xFF si no hay)
uint8_t stopNearest(const Explore &ex, int32_t latE6, int32_t lonE6, uint32_t *distM);

#define MERCHANT_ITEMS 3
struct MerchantOffer { uint8_t item; uint16_t price; };
#define LOOT_MAX 5
struct Loot {
  uint8_t count = 0;
  uint8_t item[LOOT_MAX] = {}, n[LOOT_MAX] = {};  // ya metidos en la mochila
  uint8_t berries = 0;           // bayas para el cuidado del Companero (pet.cpp)
  uint8_t eggTier = 0;           // huevo nuevo (0 = ninguno); ya en ex.eggs
  bool merchant = false;
  MerchantOffer offer[MERCHANT_ITEMS];
};
// girar la PokeStop `idx` desde (lat, lon)
StopResult stopSpin(Explore &ex, Trainer &t, Rng &rng, uint8_t idx, int32_t latE6, int32_t lonE6,
                    uint32_t now, Loot &out, uint16_t eggShinyDen = 48);
// botin de una PokeStop o Supply Drop: 3-5 objetos de la franja del Base Level
void exploreLoot(Explore &ex, Trainer &t, Rng &rng, Loot &out, uint16_t eggShinyDen = 48);
bool merchantBuy(Trainer &t, const Loot &loot, uint8_t idx);

// objeto "bayas": no va a la mochila, son para el cuidado del Companero
#define ITEM_BERRY IT_COUNT

// franja de recompensas por nivel (reglas: Rewards grow with difficulty):
// 0 (<20), 1 (20-34), 2 (35-49), 3 (50+)
uint8_t rewardTier(uint8_t level);
// objeto encontrado al nivel `level` (afuera tambien puede salir una Pepita)
void findItem(Explore &ex, Trainer &t, Rng &rng, uint8_t level, bool outside, uint32_t now,
              uint8_t &item, uint8_t &count);
// Caramelo Raro gratis: como mucho 1 al dia sumando todas las fuentes
bool freeRareCandy(Explore &ex, Trainer &t, uint32_t now);

// ---- huevos ----
// pone un huevo en un hueco libre; false si ya lleva 3
bool eggAdd(Explore &ex, Rng &rng, uint8_t tier, uint16_t shinyDen = 48);
bool eggReady(const Explore &ex, uint8_t idx);
// eclosiona: nivel max(5, Base Level - 5), confianza 100; entra al equipo o al PC
bool eggHatch(Explore &ex, Trainer &t, Rng &rng, uint8_t idx, OwnedMon *hatched = nullptr);

// ---- guardado ----
size_t exploreSaveSize();
size_t exploreSave(const Explore &ex, uint8_t *buf, size_t cap);
bool exploreLoad(Explore &ex, const uint8_t *buf, size_t len);
