#pragma once
#include <stdint.h>
#include "battle.h"

// Gimnasios de PokeTrek (reglas: "Gyms, Elite Four & legendaries" y
// "Difficulty scaling"). Los lideres escalan con el equipo del jugador.

#define NUM_GYMS 8

struct GymInfo {
  const char *leader;
  uint8_t type;          // tipo del gimnasio
  uint8_t kmToUnlock;
  uint8_t levelOffset;   // nivel del as = Base Level + offset
  uint8_t teamSize;
  uint8_t ace, aceHigh;  // as clasico y el que saca desde nivel 35
  uint8_t pool[8];       // el resto del equipo sale de aqui (formas base; 0 = fin)
};

extern const GymInfo GYMS[NUM_GYMS];

// Base Level = redondeo(0.6 * nivel mas alto + 0.4 * media del equipo)
uint8_t baseLevel(const BattleMon *party, uint8_t count);

// nivel de MT que da el lider segun el nivel de su as: 0 (<25), 1 (25-44), 2 (45+)
uint8_t gymTmTier(uint8_t aceLevel);

// Elige 4 movimientos (por nivel y, si machines, tambien MT/MO) pensando en
// dar cobertura contra los tipos de `foes`: hasta 3 de dano de tipos distintos
// y un movimiento de estado util si lo hay.
void pickMovesSmart(BattleMon &m, const BattleMon *foes, uint8_t nFoes, bool machines);

// Monta el equipo del lider `gym` (0..7) contra el equipo del jugador.
void buildGymTeam(BattleSide &leader, uint8_t gym, const BattleSide &challenger, Rng &rng);

// curacion de una Pocion segun el nivel (Pocion, Superpocion, Hiperpocion, Max.)
uint16_t potionForLevel(uint8_t level);
