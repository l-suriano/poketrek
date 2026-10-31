#pragma once
#include <stdint.h>
#include <stddef.h>
#include "battle.h"
#include "trainer.h"
#include "explore.h"

// Progreso y recompensas de PokeTrek (reglas: "Rewards, milestones &
// collectibles", las recompensas de "Gyms, Elite Four & legendaries" y el
// Daily Challenge de "Tournaments"). Lo que hace mas fuerte tiene tope; lo que
// solo se presume (skins, fondos, titulos, cintas) no.

// ---- coleccionables (sin poder en combate) ----
enum : uint8_t { SKIN_GREAT = 1, SKIN_ULTRA = 2, SKIN_SAFARI = 4, SKIN_PREMIER = 8, SKIN_MASTER = 16 };
enum : uint8_t { TITLE_DAILY_CHAMPION = 1, TITLE_FIRE_CUP_MASTER = 2, TITLE_KANTO_LEGEND = 4 };
enum : uint8_t { BG_EXCLUSIVE = 1 };

struct Progress {
  // Daily Challenge: las metas cuentan dias jugados en total, no seguidos
  uint16_t dcDays = 0;
  uint16_t dcLastDay = 0xFFFF;   // ultimo dia jugado (un intento al dia)
  uint8_t dcStreak = 0;          // dias seguidos (cada 7 dan 5 BP)
  uint16_t dcScore = 0;          // puntuacion de hoy
  bool perfectCrown = false;     // Perfect Run: 3-0 sin debilitados
  // semana (lunes a domingo): Featured Cup y Battle Shop
  uint16_t week = 0xFFFF;
  uint8_t featuredWins = 0;
  bool weeklyPrizeGiven = false;
  uint8_t candiesBought = 0;     // Caramelos Raros de la Battle Shop (3 a la semana)
  bool expShareBought = false;   // una vez en toda la partida
  // Liga y regalos unicos
  uint8_t eliteBeaten = 0;       // bit i = ELITE[i] vencido alguna vez
  bool masterBallGiven = false;
  bool mewGiven = false;
  bool mewPending = false;       // ganado con el equipo y el PC llenos
  uint8_t pendingEggs[2] = {0, 0};  // huevos ganados sin hueco libre: [raros, shiny]
  // coleccionables
  uint8_t skins = 0, backgrounds = 0, titles = 0;
  uint8_t ribbons = 0;           // bit = copa (Cup)
  uint16_t tournamentsWon = 0;
};

void progressInit(Progress &p);
uint16_t weekIndex(uint32_t now);  // semanas que empiezan el lunes 00:00
// cambia de semana si toca: reinicia Featured Cup y topes de la Battle Shop
void progressWeek(Progress &p, uint32_t now);

// probabilidad de shiny salvaje: 1 de 256, o 1 de 128 tras 30 dias del Daily Challenge
uint16_t wildShinyDen(const Progress &p);

// ---- bono de victoria limpia: x1,25 sin debilitados y otra vez x1,25 con el Companero ----
uint32_t cleanWinBonus(uint32_t amount, bool noneFainted, bool partnerInTeam);
bool partnerInParty(const Trainer &t);

// ---- gimnasios ----
// premio en MT/objetos segun el nivel del as (reglas: tabla de gimnasios)
struct GymPrize { uint8_t tm[2]; uint8_t item; uint8_t itemCount; };  // tm = numero de MT (0 = nada)
GymPrize gymPrize(uint8_t gym, uint8_t aceLevel);

struct GymResult {
  bool newBadge = false;
  GymPrize prize = {{0, 0}, 0, 0};
  uint32_t money = 0;
  ExpBonus exp = {true, false, false, false};  // para battleExp
};
// tras ganar al gimnasio `gym`: medalla y MT la primera vez; revanchas tras ser
// Campeon a mitad de dinero y XP; antes de eso, repetir no da nada
GymResult gymVictory(Trainer &t, uint8_t gym, uint8_t aceLevel, uint8_t lastLevel,
                     bool noneFainted);

// ---- Liga ----
// vencer a ELITE[idx] (0..3) o al Campeon (4): la Master Ball, una vez
void eliteVictory(Trainer &t, Progress &p, uint8_t idx);

// ---- Daily Challenge: metas y racha ----
// cuenta el dia de hoy (al empezar el primer combate). false si ya se jugo hoy.
// Da las metas de 7/14/30/50/100 dias y 5 BP por cada 7 dias seguidos.
bool dailyCountDay(Progress &p, Trainer &t, Explore &ex, Rng &rng, uint32_t now,
                   uint16_t *bpBonus = nullptr);
bool dailyPlayedToday(const Progress &p, uint32_t now);
// entrega lo ganado que esperaba hueco (huevos de metas, Mew)
void deliverPending(Progress &p, Trainer &t, Explore &ex, Rng &rng);

// ---- guardado ----
size_t progressSaveSize();
size_t progressSave(const Progress &p, uint8_t *buf, size_t cap);
bool progressLoad(Progress &p, const uint8_t *buf, size_t len);
