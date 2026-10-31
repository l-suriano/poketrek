#pragma once
#include <stdint.h>
#include <stddef.h>
#include "battle.h"

// Estado del jugador de PokeTrek fuera de las batallas: equipo y PC, experiencia,
// evoluciones, confianza, captura, mochila, dinero, medallas, Pokedex y guardado.
// Reglas: secciones "Partner, team & trust", "Levels, stats & evolution",
// "Catching" y "Economy & shops". C++ puro, como battle.cpp.

#define BOX_MAX 60
#define NUM_BADGES 8
#define DEX_BYTES 19  // 151 bits

// ---- objetos ----
enum Item : uint8_t {
  IT_POKE_BALL, IT_GREAT_BALL, IT_ULTRA_BALL, IT_MASTER_BALL,
  IT_POTION, IT_SUPER_POTION, IT_HYPER_POTION, IT_MAX_POTION, IT_FULL_RESTORE,
  IT_REVIVE, IT_MAX_REVIVE,
  IT_ANTIDOTE, IT_PARALYZE_HEAL, IT_AWAKENING, IT_BURN_HEAL, IT_ICE_HEAL, IT_FULL_HEAL,
  IT_FIRE_STONE, IT_WATER_STONE, IT_THUNDER_STONE, IT_LEAF_STONE, IT_MOON_STONE,
  IT_HP_UP, IT_PROTEIN, IT_IRON, IT_CARBOS, IT_CALCIUM,
  IT_RARE_CANDY, IT_NUGGET, IT_INCENSE, IT_LINK_CABLE, IT_EXP_SHARE,
  IT_COUNT
};
enum Pocket : uint8_t { PK_ITEMS, PK_BALLS, PK_MEDICINE, PK_KEY };

struct ItemInfo {
  const char *en, *es;
  uint16_t price;   // precio del Poke Mart en Pokedolares; 0 = no se vende ahi
  uint8_t pocket;
};
extern const ItemInfo ITEMS[IT_COUNT];
const char *itemName(uint8_t item);

// ---- un Pokemon del jugador (en el equipo o en el PC) ----
enum : uint8_t { OM_SHINY = 1, OM_PARTNER = 2, OM_TRADED = 4 };

struct OwnedMon {
  uint8_t species = 0;   // 0 = hueco vacio
  uint8_t level = 1;
  uint32_t exp = 0;
  uint8_t genes[5] = {100, 100, 100, 100, 100};  // HP, Atk, Def, Spe, Spc (90..110 %)
  uint16_t statExp[5] = {0, 0, 0, 0, 0};
  uint8_t moves[4] = {0, 0, 0, 0};
  uint8_t pp[4] = {0, 0, 0, 0};
  uint16_t hp = 0;       // PS actuales; 0 = debilitado
  uint8_t status = STS_NONE;
  uint8_t trust = 100;
  uint8_t trustToday = 0;  // confianza ganada hoy por combates (tope 10)
  uint8_t flags = 0;
};

struct Trainer {
  OwnedMon party[PARTY_MAX];
  uint8_t partyCount = 0;
  OwnedMon box[BOX_MAX];
  uint8_t boxCount = 0;
  uint16_t bag[IT_COUNT] = {};
  uint8_t tms[NUM_MACHINES] = {};  // MT de un solo uso (gen 1); las MO no se gastan
  uint32_t money = 3000;
  uint16_t bp = 0;                 // Battle Points
  uint8_t badges = 0;              // bit i = medalla del gimnasio i
  bool champion = false;
  uint8_t seen[DEX_BYTES] = {}, caught[DEX_BYTES] = {};
};

// ---- experiencia y niveles (curvas de gen 1) ----
uint32_t expForLevel(uint8_t growth, uint8_t level);

// XP de un combate (reglas: Battle XP): experiencia base * nivel / 7 por los bonos
struct ExpBonus { bool trainer, firstLeaderWin, featured, rematch; };
uint32_t battleExp(uint8_t defeatedSpecies, uint8_t defeatedLevel, ExpBonus bonus);

#define MAX_NEW_MOVES 8
struct LevelUp {
  uint8_t levels = 0;                    // niveles subidos
  uint8_t newMoves[MAX_NEW_MOVES] = {};  // movimientos que quiere aprender
  uint8_t newMoveCount = 0;
  uint8_t evolveTo = 0;                  // evolucion por nivel disponible (0 = ninguna)
};
LevelUp gainExp(OwnedMon &m, uint32_t exp);
// Stat Exp de gen 1: suma los stats base del derrotado
void addStatExp(OwnedMon &m, uint8_t defeatedSpecies);
// aprende `move` en un hueco libre o sustituyendo `replaceSlot` (0..3). false si no cabe.
bool learnMove(OwnedMon &m, uint8_t move, uint8_t replaceSlot = 0xFF);
bool canLearnMachine(uint8_t species, uint8_t machineIdx);

// ---- crear y convertir ----
void ownedInit(OwnedMon &m, uint8_t species, uint8_t level, Rng &rng);
uint16_t ownedMaxHp(const OwnedMon &m);
void toBattle(const OwnedMon &m, BattleMon &out);
void fromBattle(OwnedMon &m, const BattleMon &b);  // copia PS, PP y estado al acabar

// ---- evolucion ----
uint8_t levelEvolution(const OwnedMon &m);           // 0 si no toca
uint8_t stoneEvolution(uint8_t species, uint8_t stoneItem);
uint8_t tradeEvolution(uint8_t species);
void evolve(OwnedMon &m, uint8_t newSpecies);        // sube los PS lo que sube el maximo

// ---- confianza (reglas: Trust and obedience) ----
uint8_t startingTrust(uint8_t caughtLevel, uint8_t highestPartyLevel, bool traded);
void trustAfterBattle(OwnedMon &m);    // +2, tope +10 al dia
void trustPerKm(OwnedMon &m, uint16_t km);
void trustTournamentWin(OwnedMon &m);  // +5
void trustNewDay(Trainer &t);

// ---- captura ----
uint8_t ballMultX10(uint8_t ballItem);  // 10, 15, 20; 0 = Master Ball
// tira la bola: true si lo atrapa; `shakes` = sacudidas a mostrar (0..3)
bool catchAttempt(Rng &rng, const BattleMon &wild, uint8_t ballItem, uint8_t throwX10, uint8_t *shakes);
// mete al Pokemon atrapado en el equipo o en el PC. false si los dos estan llenos.
bool addCaught(Trainer &t, const BattleMon &wild, Rng &rng, bool shiny = false);
bool addOwned(Trainer &t, const OwnedMon &m);

// ---- equipo ----
uint8_t highestPartyLevel(const Trainer &t);
bool anyAlive(const Trainer &t);
bool depositToBox(Trainer &t, uint8_t partyIdx);  // el compañero no se puede dejar
bool withdrawFromBox(Trainer &t, uint8_t boxIdx);
void healAll(Trainer &t);                          // Pokemon Center

// ---- objetos fuera de combate ----
enum UseResult : uint8_t { USE_OK, USE_NO_ITEM, USE_NO_EFFECT, USE_EVOLVED, USE_LEVEL_UP };
UseResult useItemOn(Trainer &t, uint8_t item, OwnedMon &m, LevelUp *lv = nullptr);
void addItem(Trainer &t, uint8_t item, uint16_t n = 1);

// ---- dinero, medallas, Pokedex ----
bool spend(Trainer &t, uint32_t amount);
void earn(Trainer &t, uint32_t amount);
uint32_t prizeMoney(uint8_t classBase, uint8_t lastLevel);
void blackout(Trainer &t);                 // pierde el 10 % del dinero
uint8_t badgeCount(const Trainer &t);
bool martSells(const Trainer &t, uint8_t item);  // segun las medallas (reglas: Poke Mart)
void dexMark(uint8_t *bits, uint8_t species);
bool dexHas(const uint8_t *bits, uint8_t species);
uint8_t dexCount(const uint8_t *bits);

// ---- guardado (cabecera + CRC32; el struct va tal cual) ----
#define SAVE_HEADER 16  // magic 4 + version 2 + reservado 2 + tamano 4 + crc 4
size_t saveBlob(const char magic[4], uint16_t version, const void *data, size_t size,
                uint8_t *buf, size_t cap);
bool loadBlob(const char magic[4], uint16_t version, void *data, size_t size,
              const uint8_t *buf, size_t len);
size_t trainerSaveSize();
size_t trainerSave(const Trainer &t, uint8_t *buf, size_t cap);
bool trainerLoad(Trainer &t, const uint8_t *buf, size_t len);
