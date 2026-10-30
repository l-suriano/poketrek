#pragma once
#include <stdint.h>
#include "battle.h"

// Gimnasios de PokeTrek (reglas: "Gyms, Elite Four & legendaries" y
// "Difficulty scaling"). Los lideres escalan con el equipo del jugador.

#define NUM_GYMS 8
#define GYM_PCT_TABLE 127  // "usa el levelPct de GYMS"

struct GymInfo {
  const char *leader;
  uint8_t type;          // tipo del gimnasio
  uint8_t kmToUnlock;
  // nivel del as = Base Level * (100 + levelPct) / 100, con un % por tramo de
  // Base Level (<25, 25-44, 45+): la IA y las evoluciones del lider cambian
  // por tramos, asi que un % unico no deja la dificultad pareja
  int8_t levelPct[3];
  uint8_t aiTier;        // fijo por gimnasio: si dependiera del nivel, escalar
                         // el nivel subiria la IA a saltos
  uint8_t teamSize;
  uint8_t ace, aceHigh;  // as clasico y el que saca desde nivel 35
  uint8_t pool[8];       // el resto del equipo sale de aqui (formas base; 0 = fin)
};

extern const GymInfo GYMS[NUM_GYMS];

// Base Level = redondeo(0.6 * nivel mas alto + 0.4 * media del equipo)
uint8_t baseLevel(const BattleMon *party, uint8_t count);

// nivel de MT que da el lider segun el nivel de su as: 0 (<25), 1 (25-44), 2 (45+)
uint8_t gymTmTier(uint8_t aceLevel);
// tramo de Base Level para levelPct: 0 (<25), 1 (25-44), 2 (45+)
uint8_t gymBand(uint8_t bl);

// Elige 4 movimientos (por nivel y, si machines, tambien MT/MO) pensando en
// dar cobertura contra los tipos de `foes`: hasta 3 de dano de tipos distintos
// y un movimiento de estado util si lo hay.
void pickMovesSmart(BattleMon &m, const BattleMon *foes, uint8_t nFoes, bool machines);

// nivel del as del lider contra un Base Level dado
uint8_t gymAceLevel(uint8_t gym, uint8_t bl, int8_t pct = GYM_PCT_TABLE);

// Monta el equipo del lider `gym` (0..7) contra el equipo del jugador.
// `pct` sustituye al de la tabla (lo usa el simulador para afinar).
void buildGymTeam(BattleSide &leader, uint8_t gym, const BattleSide &challenger, Rng &rng,
                  int8_t pct = GYM_PCT_TABLE);

// curacion de una Pocion segun el nivel (Pocion, Superpocion, Hiperpocion, Max.)
uint16_t potionForLevel(uint8_t level);
