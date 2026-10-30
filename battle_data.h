#pragma once
#include <stdint.h>
#include "i18n.h"  // gLang

// GENERADO por tools/gen_battle.py desde tools/battle_data.py - no editar

// ---- tipos (15, orden de gen 1) ----
enum BType : uint8_t {
  T_NORMAL,
  T_FIGHTING,
  T_FLYING,
  T_POISON,
  T_GROUND,
  T_ROCK,
  T_BUG,
  T_GHOST,
  T_FIRE,
  T_WATER,
  T_GRASS,
  T_ELECTRIC,
  T_PSYCHIC,
  T_ICE,
  T_DRAGON,
  T_COUNT,
  T_NONE = 0xFF  // segundo tipo vacio
};

static const char *const TYPE_NAME_EN[T_COUNT] = {
  "Normal", "Fighting", "Flying", "Poison", "Ground", "Rock", "Bug", "Ghost", "Fire", "Water", "Grass", "Electric", "Psychic", "Ice", "Dragon"
};
static const char *const TYPE_NAME_ES[T_COUNT] = {
  "Normal", "Lucha", "Volador", "Veneno", "Tierra", "Roca", "Bicho", "Fantasma", "Fuego", "Agua", "Planta", "El\202ctrico", "Ps\241quico", "Hielo", "Drag\242n"
};

// eficacia x2: 0 inmune, 1 = x0.5, 2 = x1, 4 = x2 [atacante][defensor].
// Gen 1 tal cual salvo Fantasma > Psiquico, que aqui es x2 (bug corregido).
static const uint8_t TYPE_EFF[T_COUNT][T_COUNT] = {
  { 2, 2, 2, 2, 2, 1, 2, 0, 2, 2, 2, 2, 2, 2, 2 },  // normal
  { 4, 2, 1, 1, 2, 4, 1, 0, 2, 2, 2, 2, 1, 4, 2 },  // fighting
  { 2, 4, 2, 2, 2, 1, 4, 2, 2, 2, 4, 1, 2, 2, 2 },  // flying
  { 2, 2, 2, 1, 1, 1, 4, 1, 2, 2, 4, 2, 2, 2, 2 },  // poison
  { 2, 2, 0, 4, 2, 4, 1, 2, 4, 2, 1, 4, 2, 2, 2 },  // ground
  { 2, 1, 4, 2, 1, 2, 4, 2, 4, 2, 2, 2, 2, 4, 2 },  // rock
  { 2, 1, 1, 4, 2, 2, 2, 1, 1, 2, 4, 2, 4, 2, 2 },  // bug
  { 0, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 4, 2, 2 },  // ghost
  { 2, 2, 2, 2, 2, 1, 4, 2, 1, 1, 4, 2, 2, 4, 1 },  // fire
  { 2, 2, 2, 2, 4, 4, 2, 2, 4, 1, 1, 2, 2, 2, 1 },  // water
  { 2, 2, 1, 1, 4, 4, 1, 2, 1, 4, 1, 2, 2, 2, 1 },  // grass
  { 2, 2, 4, 2, 0, 2, 2, 2, 2, 4, 1, 1, 2, 2, 1 },  // electric
  { 2, 4, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2 },  // psychic
  { 2, 2, 4, 2, 4, 2, 2, 2, 2, 1, 4, 2, 2, 1, 4 },  // ice
  { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4 },  // dragon
};

enum Growth : uint8_t { G_FAST, G_MEDIUM, G_MEDIUM_SLOW, G_SLOW };
enum BStat : uint8_t { ST_NONE = 0, ST_ATTACK, ST_DEFENSE, ST_SPEED, ST_SPECIAL, ST_ACCURACY, ST_EVASION };
enum Ailment : uint8_t { AIL_NONE = 0, AIL_BURN, AIL_CONFUSION, AIL_DISABLE, AIL_FREEZE, AIL_LEECH_SEED, AIL_PARALYSIS, AIL_POISON, AIL_SLEEP, AIL_TRAP, AIL_UNKNOWN };
enum MoveCat : uint8_t { MC_AILMENT, MC_DAMAGE, MC_DAMAGE_AILMENT, MC_DAMAGE_HEAL, MC_DAMAGE_LOWER, MC_FIELD_EFFECT, MC_FORCE_SWITCH, MC_HEAL, MC_NET_GOOD_STATS, MC_OHKO, MC_UNIQUE, MC_WHOLE_FIELD_EFFECT };
enum Stone : uint8_t { STONE_FIRE, STONE_WATER, STONE_THUNDER, STONE_LEAF, STONE_MOON };
enum EvoMethod : uint8_t { EVO_LEVEL, EVO_STONE, EVO_TRADE };

// ---- movimientos: indice = id de PokeAPI (1..165), 0 sin usar ----
struct MoveInfo {
  const char *en, *es;
  uint8_t type;
  uint8_t power;      // 0 = sin poder fijo (estado o dano especial)
  uint8_t accuracy;   // 0 = nunca falla
  uint8_t pp;
  int8_t priority;
  uint8_t category;   // MoveCat
  uint8_t ailment;    // Ailment que puede causar
  uint8_t chance;     // % del efecto secundario; 0 = siempre
  uint8_t flinch;     // % de retroceso
  uint8_t highCrit;   // 1 = critico alto (Cuchillada, Hoja Afilada...)
  int8_t drain;       // % del dano que recupera (<0 = retroceso)
  int8_t healing;     // % de PS que cura
  uint8_t minHits, maxHits, minTurns, maxTurns;
  uint8_t stat1; int8_t delta1;
  uint8_t stat2; int8_t delta2;
  uint8_t selfStat;   // 1 = los cambios de stat son para el usuario
};

#define NUM_MOVES 165
static const MoveInfo MOVE_TBL[NUM_MOVES + 1] = {
  { "-", "-", T_NORMAL, 0, 0, 0, 0, 0, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },
  { "Pound", "Destructor", T_NORMAL, 40, 100, 35, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 1 pound
  { "Karate Chop", "Golpe K\240rate", T_NORMAL, 50, 100, 25, 0, MC_DAMAGE, AIL_NONE, 0, 0, 1, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 2 karate-chop
  { "Double Slap", "Doble Bofet\242n", T_NORMAL, 15, 85, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 2, 5, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 3 double-slap
  { "Comet Punch", "Pu\244o Cometa", T_NORMAL, 18, 85, 15, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 2, 5, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 4 comet-punch
  { "Mega Punch", "Megapu\244o", T_NORMAL, 80, 85, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 5 mega-punch
  { "Pay Day", "D\241a de Pago", T_NORMAL, 40, 100, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 6 pay-day
  { "Fire Punch", "Pu\244o Fuego", T_FIRE, 75, 100, 15, 0, MC_DAMAGE_AILMENT, AIL_BURN, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 7 fire-punch
  { "Ice Punch", "Pu\244o Hielo", T_ICE, 75, 100, 15, 0, MC_DAMAGE_AILMENT, AIL_FREEZE, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 8 ice-punch
  { "Thunder Punch", "Pu\244o Trueno", T_ELECTRIC, 75, 100, 15, 0, MC_DAMAGE_AILMENT, AIL_PARALYSIS, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 9 thunder-punch
  { "Scratch", "Ara\244azo", T_NORMAL, 40, 100, 35, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 10 scratch
  { "Vise Grip", "Agarre", T_NORMAL, 55, 100, 30, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 11 vice-grip
  { "Guillotine", "Guillotina", T_NORMAL, 0, 30, 5, 0, MC_OHKO, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 12 guillotine
  { "Razor Wind", "Viento Cortante", T_NORMAL, 80, 75, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 1, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 13 razor-wind
  { "Swords Dance", "Danza Espada", T_NORMAL, 0, 0, 30, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_ATTACK, 2, ST_NONE, 0, 1 },  // 14 swords-dance
  { "Cut", "Corte", T_NORMAL, 50, 95, 30, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 15 cut
  { "Gust", "Tornado", T_NORMAL, 40, 100, 35, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 16 gust
  { "Wing Attack", "Ataque Ala", T_FLYING, 35, 100, 35, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 17 wing-attack
  { "Whirlwind", "Remolino", T_NORMAL, 0, 85, 20, -6, MC_FORCE_SWITCH, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 18 whirlwind
  { "Fly", "Vuelo", T_FLYING, 70, 95, 15, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 19 fly
  { "Bind", "Atadura", T_NORMAL, 15, 75, 20, 0, MC_DAMAGE_AILMENT, AIL_TRAP, 100, 0, 0, 0, 0, 0, 0, 5, 6, ST_NONE, 0, ST_NONE, 0, 0 },  // 20 bind
  { "Slam", "Atizar", T_NORMAL, 80, 75, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 21 slam
  { "Vine Whip", "L\240tigo Cepa", T_GRASS, 35, 100, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 22 vine-whip
  { "Stomp", "Pisot\242n", T_NORMAL, 65, 100, 20, 0, MC_DAMAGE, AIL_NONE, 30, 30, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 23 stomp
  { "Double Kick", "Doble Patada", T_FIGHTING, 30, 100, 30, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 2, 2, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 24 double-kick
  { "Mega Kick", "Megapatada", T_NORMAL, 120, 75, 5, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 25 mega-kick
  { "Jump Kick", "Patada Salto", T_FIGHTING, 70, 95, 25, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 26 jump-kick
  { "Rolling Kick", "Patada Giro", T_FIGHTING, 60, 85, 15, 0, MC_DAMAGE, AIL_NONE, 30, 30, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 27 rolling-kick
  { "Sand Attack", "Ataque Arena", T_NORMAL, 0, 100, 15, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_ACCURACY, -1, ST_NONE, 0, 0 },  // 28 sand-attack
  { "Headbutt", "Golpe Cabeza", T_NORMAL, 70, 100, 15, 0, MC_DAMAGE, AIL_NONE, 30, 30, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 29 headbutt
  { "Horn Attack", "Cornada", T_NORMAL, 65, 100, 25, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 30 horn-attack
  { "Fury Attack", "Ataque Furia", T_NORMAL, 15, 85, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 2, 5, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 31 fury-attack
  { "Horn Drill", "Perforador", T_NORMAL, 0, 30, 5, 0, MC_OHKO, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 32 horn-drill
  { "Tackle", "Placaje", T_NORMAL, 35, 95, 35, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 33 tackle
  { "Body Slam", "Golpe Cuerpo", T_NORMAL, 85, 100, 15, 0, MC_DAMAGE_AILMENT, AIL_PARALYSIS, 30, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 34 body-slam
  { "Wrap", "Constricci\242n", T_NORMAL, 15, 85, 20, 0, MC_DAMAGE_AILMENT, AIL_TRAP, 100, 0, 0, 0, 0, 0, 0, 2, 4, ST_NONE, 0, ST_NONE, 0, 0 },  // 35 wrap
  { "Take Down", "Derribo", T_NORMAL, 90, 85, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, -25, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 36 take-down
  { "Thrash", "Sa\244a", T_NORMAL, 90, 100, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 37 thrash
  { "Double-Edge", "Doble Filo", T_NORMAL, 100, 100, 15, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, -33, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 38 double-edge
  { "Tail Whip", "Agitacola", T_NORMAL, 0, 100, 30, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_DEFENSE, -1, ST_NONE, 0, 0 },  // 39 tail-whip
  { "Poison Sting", "Picotazo Veneno", T_POISON, 15, 100, 35, 0, MC_DAMAGE_AILMENT, AIL_POISON, 20, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 40 poison-sting
  { "Twineedle", "Doble Ataque", T_BUG, 25, 100, 20, 0, MC_DAMAGE_AILMENT, AIL_POISON, 20, 0, 0, 0, 0, 2, 2, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 41 twineedle
  { "Pin Missile", "Pin Misil", T_BUG, 14, 85, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 2, 5, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 42 pin-missile
  { "Leer", "Malicioso", T_NORMAL, 0, 100, 30, 0, MC_NET_GOOD_STATS, AIL_NONE, 100, 0, 0, 0, 0, 0, 0, 0, 0, ST_DEFENSE, -1, ST_NONE, 0, 0 },  // 43 leer
  { "Bite", "Mordisco", T_NORMAL, 60, 100, 25, 0, MC_DAMAGE, AIL_NONE, 10, 10, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 44 bite
  { "Growl", "Gru\244ido", T_NORMAL, 0, 100, 40, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_ATTACK, -1, ST_NONE, 0, 0 },  // 45 growl
  { "Roar", "Rugido", T_NORMAL, 0, 100, 20, -6, MC_FORCE_SWITCH, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 46 roar
  { "Sing", "Canto", T_NORMAL, 0, 55, 15, 0, MC_AILMENT, AIL_SLEEP, 0, 0, 0, 0, 0, 0, 0, 2, 4, ST_NONE, 0, ST_NONE, 0, 0 },  // 47 sing
  { "Supersonic", "Supers\242nico", T_NORMAL, 0, 55, 20, 0, MC_AILMENT, AIL_CONFUSION, 0, 0, 0, 0, 0, 0, 0, 2, 5, ST_NONE, 0, ST_NONE, 0, 0 },  // 48 supersonic
  { "Sonic Boom", "Bomba S\242nica", T_NORMAL, 0, 90, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 49 sonic-boom
  { "Disable", "Anulaci\242n", T_NORMAL, 0, 55, 20, 0, MC_UNIQUE, AIL_DISABLE, 0, 0, 0, 0, 0, 0, 0, 4, 4, ST_NONE, 0, ST_NONE, 0, 0 },  // 50 disable
  { "Acid", "Acido", T_POISON, 40, 100, 30, 0, MC_DAMAGE_LOWER, AIL_NONE, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_SPECIAL, -1, ST_NONE, 0, 0 },  // 51 acid
  { "Ember", "Ascuas", T_FIRE, 40, 100, 25, 0, MC_DAMAGE_AILMENT, AIL_BURN, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 52 ember
  { "Flamethrower", "Lanzallamas", T_FIRE, 95, 100, 15, 0, MC_DAMAGE_AILMENT, AIL_BURN, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 53 flamethrower
  { "Mist", "Neblina", T_ICE, 0, 0, 30, 0, MC_FIELD_EFFECT, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 54 mist
  { "Water Gun", "Pistola Agua", T_WATER, 40, 100, 25, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 55 water-gun
  { "Hydro Pump", "Hidrobomba", T_WATER, 120, 80, 5, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 56 hydro-pump
  { "Surf", "Surf", T_WATER, 95, 100, 15, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 57 surf
  { "Ice Beam", "Rayo Hielo", T_ICE, 95, 100, 10, 0, MC_DAMAGE_AILMENT, AIL_FREEZE, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 58 ice-beam
  { "Blizzard", "Ventisca", T_ICE, 120, 90, 5, 0, MC_DAMAGE_AILMENT, AIL_FREEZE, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 59 blizzard
  { "Psybeam", "Psicorrayo", T_PSYCHIC, 65, 100, 20, 0, MC_DAMAGE_AILMENT, AIL_CONFUSION, 10, 0, 0, 0, 0, 0, 0, 2, 5, ST_NONE, 0, ST_NONE, 0, 0 },  // 60 psybeam
  { "Bubble Beam", "Rayo Burbuja", T_WATER, 65, 100, 20, 0, MC_DAMAGE_LOWER, AIL_NONE, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_SPEED, -1, ST_NONE, 0, 0 },  // 61 bubble-beam
  { "Aurora Beam", "Rayo Aurora", T_ICE, 65, 100, 20, 0, MC_DAMAGE_LOWER, AIL_NONE, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_ATTACK, -1, ST_NONE, 0, 0 },  // 62 aurora-beam
  { "Hyper Beam", "Hiperrayo", T_NORMAL, 150, 90, 5, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 63 hyper-beam
  { "Peck", "Picotazo", T_FLYING, 35, 100, 35, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 64 peck
  { "Drill Peck", "Pico Taladro", T_FLYING, 80, 100, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 65 drill-peck
  { "Submission", "Sumisi\242n", T_FIGHTING, 80, 80, 25, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, -25, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 66 submission
  { "Low Kick", "Patada Baja", T_FIGHTING, 50, 90, 20, 0, MC_DAMAGE, AIL_NONE, 30, 30, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 67 low-kick
  { "Counter", "Contraataque", T_FIGHTING, 0, 100, 20, -5, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 68 counter
  { "Seismic Toss", "S\241smico", T_FIGHTING, 0, 100, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 69 seismic-toss
  { "Strength", "Fuerza", T_NORMAL, 80, 100, 15, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 70 strength
  { "Absorb", "Absorber", T_GRASS, 20, 100, 20, 0, MC_DAMAGE_HEAL, AIL_NONE, 0, 0, 0, 50, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 71 absorb
  { "Mega Drain", "Megaagotar", T_GRASS, 40, 100, 10, 0, MC_DAMAGE_HEAL, AIL_NONE, 0, 0, 0, 50, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 72 mega-drain
  { "Leech Seed", "Drenadoras", T_GRASS, 0, 90, 10, 0, MC_AILMENT, AIL_LEECH_SEED, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 73 leech-seed
  { "Growth", "Desarrollo", T_NORMAL, 0, 0, 40, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_SPECIAL, 1, ST_NONE, 0, 1 },  // 74 growth
  { "Razor Leaf", "Hoja Afilada", T_GRASS, 55, 95, 25, 0, MC_DAMAGE, AIL_NONE, 0, 0, 1, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 75 razor-leaf
  { "Solar Beam", "Rayo Solar", T_GRASS, 120, 100, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 76 solar-beam
  { "Poison Powder", "Polvo Veneno", T_POISON, 0, 75, 35, 0, MC_AILMENT, AIL_POISON, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 77 poison-powder
  { "Stun Spore", "Paralizador", T_GRASS, 0, 75, 30, 0, MC_AILMENT, AIL_PARALYSIS, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 78 stun-spore
  { "Sleep Powder", "Somn\241fero", T_GRASS, 0, 75, 15, 0, MC_AILMENT, AIL_SLEEP, 0, 0, 0, 0, 0, 0, 0, 2, 4, ST_NONE, 0, ST_NONE, 0, 0 },  // 79 sleep-powder
  { "Petal Dance", "Danza P\202talo", T_GRASS, 70, 100, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 80 petal-dance
  { "String Shot", "Disparo Demora", T_BUG, 0, 95, 40, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_SPEED, -2, ST_NONE, 0, 0 },  // 81 string-shot
  { "Dragon Rage", "Furia Drag\242n", T_DRAGON, 0, 100, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 82 dragon-rage
  { "Fire Spin", "Giro Fuego", T_FIRE, 15, 70, 15, 0, MC_DAMAGE_AILMENT, AIL_TRAP, 100, 0, 0, 0, 0, 0, 0, 5, 6, ST_NONE, 0, ST_NONE, 0, 0 },  // 83 fire-spin
  { "Thunder Shock", "Impactrueno", T_ELECTRIC, 40, 100, 30, 0, MC_DAMAGE_AILMENT, AIL_PARALYSIS, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 84 thunder-shock
  { "Thunderbolt", "Rayo", T_ELECTRIC, 95, 100, 15, 0, MC_DAMAGE_AILMENT, AIL_PARALYSIS, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 85 thunderbolt
  { "Thunder Wave", "Onda Trueno", T_ELECTRIC, 0, 100, 20, 0, MC_AILMENT, AIL_PARALYSIS, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 86 thunder-wave
  { "Thunder", "Trueno", T_ELECTRIC, 120, 70, 10, 0, MC_DAMAGE_AILMENT, AIL_PARALYSIS, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 87 thunder
  { "Rock Throw", "Lanzarrocas", T_ROCK, 50, 65, 15, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 88 rock-throw
  { "Earthquake", "Terremoto", T_GROUND, 100, 100, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 89 earthquake
  { "Fissure", "Fisura", T_GROUND, 0, 30, 5, 0, MC_OHKO, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 90 fissure
  { "Dig", "Excavar", T_GROUND, 100, 100, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 91 dig
  { "Toxic", "T\242xico", T_POISON, 0, 85, 10, 0, MC_AILMENT, AIL_POISON, 0, 0, 0, 0, 0, 0, 0, 15, 15, ST_NONE, 0, ST_NONE, 0, 0 },  // 92 toxic
  { "Confusion", "Confusi\242n", T_PSYCHIC, 50, 100, 25, 0, MC_DAMAGE_AILMENT, AIL_CONFUSION, 10, 0, 0, 0, 0, 0, 0, 2, 5, ST_NONE, 0, ST_NONE, 0, 0 },  // 93 confusion
  { "Psychic", "Ps\241quico", T_PSYCHIC, 90, 100, 10, 0, MC_DAMAGE_LOWER, AIL_NONE, 30, 0, 0, 0, 0, 0, 0, 0, 0, ST_SPECIAL, -1, ST_NONE, 0, 0 },  // 94 psychic
  { "Hypnosis", "Hipnosis", T_PSYCHIC, 0, 60, 20, 0, MC_AILMENT, AIL_SLEEP, 0, 0, 0, 0, 0, 0, 0, 2, 4, ST_NONE, 0, ST_NONE, 0, 0 },  // 95 hypnosis
  { "Meditate", "Meditaci\242n", T_PSYCHIC, 0, 0, 40, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_ATTACK, 1, ST_NONE, 0, 1 },  // 96 meditate
  { "Agility", "Agilidad", T_PSYCHIC, 0, 0, 30, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_SPEED, 2, ST_NONE, 0, 1 },  // 97 agility
  { "Quick Attack", "Ataque R\240pido", T_NORMAL, 40, 100, 30, 1, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 98 quick-attack
  { "Rage", "Furia", T_NORMAL, 20, 100, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 99 rage
  { "Teleport", "Teletransporte", T_PSYCHIC, 0, 0, 20, -6, MC_UNIQUE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 100 teleport
  { "Night Shade", "Tinieblas", T_GHOST, 0, 100, 15, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 101 night-shade
  { "Mimic", "Mim\202tico", T_NORMAL, 0, 0, 10, 0, MC_UNIQUE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 102 mimic
  { "Screech", "Chirrido", T_NORMAL, 0, 85, 40, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_DEFENSE, -2, ST_NONE, 0, 0 },  // 103 screech
  { "Double Team", "Doble Equipo", T_NORMAL, 0, 0, 15, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_EVASION, 1, ST_NONE, 0, 1 },  // 104 double-team
  { "Recover", "Recuperaci\242n", T_NORMAL, 0, 0, 20, 0, MC_HEAL, AIL_NONE, 0, 0, 0, 0, 50, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 105 recover
  { "Harden", "Fortaleza", T_NORMAL, 0, 0, 30, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_DEFENSE, 1, ST_NONE, 0, 1 },  // 106 harden
  { "Minimize", "Reducci\242n", T_NORMAL, 0, 0, 20, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_EVASION, 2, ST_NONE, 0, 1 },  // 107 minimize
  { "Smokescreen", "Pantalla de Humo", T_NORMAL, 0, 100, 20, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_ACCURACY, -1, ST_NONE, 0, 0 },  // 108 smokescreen
  { "Confuse Ray", "Rayo Confuso", T_GHOST, 0, 100, 10, 0, MC_AILMENT, AIL_CONFUSION, 0, 0, 0, 0, 0, 0, 0, 2, 5, ST_NONE, 0, ST_NONE, 0, 0 },  // 109 confuse-ray
  { "Withdraw", "Refugio", T_WATER, 0, 0, 40, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_DEFENSE, 1, ST_NONE, 0, 1 },  // 110 withdraw
  { "Defense Curl", "Rizo Defensa", T_NORMAL, 0, 0, 40, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_DEFENSE, 1, ST_NONE, 0, 1 },  // 111 defense-curl
  { "Barrier", "Barrera", T_PSYCHIC, 0, 0, 30, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_DEFENSE, 2, ST_NONE, 0, 1 },  // 112 barrier
  { "Light Screen", "Pantalla de Luz", T_PSYCHIC, 0, 0, 30, 0, MC_FIELD_EFFECT, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 113 light-screen
  { "Haze", "Niebla", T_ICE, 0, 0, 30, 0, MC_WHOLE_FIELD_EFFECT, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 114 haze
  { "Reflect", "Reflejo", T_PSYCHIC, 0, 0, 20, 0, MC_FIELD_EFFECT, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 115 reflect
  { "Focus Energy", "Foco Energ\241a", T_NORMAL, 0, 0, 30, 0, MC_UNIQUE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 116 focus-energy
  { "Bide", "Venganza", T_NORMAL, 0, 0, 10, 1, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 117 bide
  { "Metronome", "Metr\242nomo", T_NORMAL, 0, 0, 10, 0, MC_UNIQUE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 118 metronome
  { "Mirror Move", "Espejo", T_FLYING, 0, 0, 20, 0, MC_UNIQUE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 119 mirror-move
  { "Self-Destruct", "Autodestrucci\242n", T_NORMAL, 130, 100, 5, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 120 self-destruct
  { "Egg Bomb", "Bomba Huevo", T_NORMAL, 100, 75, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 121 egg-bomb
  { "Lick", "Leng\201etazo", T_GHOST, 20, 100, 30, 0, MC_DAMAGE_AILMENT, AIL_PARALYSIS, 30, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 122 lick
  { "Smog", "Poluci\242n", T_POISON, 20, 70, 20, 0, MC_DAMAGE_AILMENT, AIL_POISON, 40, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 123 smog
  { "Sludge", "Residuos", T_POISON, 65, 100, 20, 0, MC_DAMAGE_AILMENT, AIL_POISON, 30, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 124 sludge
  { "Bone Club", "Hueso Palo", T_GROUND, 65, 85, 20, 0, MC_DAMAGE, AIL_NONE, 10, 10, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 125 bone-club
  { "Fire Blast", "Llamarada", T_FIRE, 120, 85, 5, 0, MC_DAMAGE_AILMENT, AIL_BURN, 30, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 126 fire-blast
  { "Waterfall", "Cascada", T_WATER, 80, 100, 15, 0, MC_DAMAGE, AIL_NONE, 20, 20, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 127 waterfall
  { "Clamp", "Tenaza", T_WATER, 35, 75, 10, 0, MC_DAMAGE_AILMENT, AIL_TRAP, 100, 0, 0, 0, 0, 0, 0, 5, 6, ST_NONE, 0, ST_NONE, 0, 0 },  // 128 clamp
  { "Swift", "Meteoros", T_NORMAL, 60, 0, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 129 swift
  { "Skull Bash", "Cabezazo", T_NORMAL, 100, 100, 15, 0, MC_DAMAGE, AIL_NONE, 100, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 130 skull-bash
  { "Spike Cannon", "Clavo Ca\244\242n", T_NORMAL, 20, 100, 15, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 2, 5, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 131 spike-cannon
  { "Constrict", "Restricci\242n", T_NORMAL, 10, 100, 35, 0, MC_DAMAGE_LOWER, AIL_NONE, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_SPEED, -1, ST_NONE, 0, 0 },  // 132 constrict
  { "Amnesia", "Amnesia", T_PSYCHIC, 0, 0, 20, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_SPECIAL, 2, ST_NONE, 0, 1 },  // 133 amnesia
  { "Kinesis", "Kin\202tico", T_PSYCHIC, 0, 80, 15, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_ACCURACY, -1, ST_NONE, 0, 0 },  // 134 kinesis
  { "Soft-Boiled", "Ovocuraci\242n", T_NORMAL, 0, 0, 5, 0, MC_HEAL, AIL_NONE, 0, 0, 0, 0, 50, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 135 soft-boiled
  { "High Jump Kick", "Patada Salto Alta", T_FIGHTING, 85, 90, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 136 high-jump-kick
  { "Glare", "Deslumbrar", T_NORMAL, 0, 75, 30, 0, MC_AILMENT, AIL_PARALYSIS, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 137 glare
  { "Dream Eater", "Comesue\244os", T_PSYCHIC, 100, 100, 15, 0, MC_DAMAGE_HEAL, AIL_NONE, 0, 0, 0, 50, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 138 dream-eater
  { "Poison Gas", "Gas Venenoso", T_POISON, 0, 55, 40, 0, MC_AILMENT, AIL_POISON, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 139 poison-gas
  { "Barrage", "Bombardeo", T_NORMAL, 15, 85, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 2, 5, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 140 barrage
  { "Leech Life", "Chupavidas", T_BUG, 20, 100, 15, 0, MC_DAMAGE_HEAL, AIL_NONE, 0, 0, 0, 50, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 141 leech-life
  { "Lovely Kiss", "Beso Amoroso", T_NORMAL, 0, 75, 10, 0, MC_AILMENT, AIL_SLEEP, 0, 0, 0, 0, 0, 0, 0, 2, 4, ST_NONE, 0, ST_NONE, 0, 0 },  // 142 lovely-kiss
  { "Sky Attack", "Ataque A\202reo", T_FLYING, 140, 90, 5, 0, MC_DAMAGE, AIL_NONE, 10, 10, 1, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 143 sky-attack
  { "Transform", "Transformaci\242n", T_NORMAL, 0, 0, 10, 0, MC_UNIQUE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 144 transform
  { "Bubble", "Burbuja", T_WATER, 20, 100, 30, 0, MC_DAMAGE_LOWER, AIL_NONE, 10, 0, 0, 0, 0, 0, 0, 0, 0, ST_SPEED, -1, ST_NONE, 0, 0 },  // 145 bubble
  { "Dizzy Punch", "Pu\244o Mareo", T_NORMAL, 70, 100, 10, 0, MC_DAMAGE_AILMENT, AIL_CONFUSION, 0, 0, 0, 0, 0, 0, 0, 2, 5, ST_NONE, 0, ST_NONE, 0, 0 },  // 146 dizzy-punch
  { "Spore", "Espora", T_GRASS, 0, 100, 15, 0, MC_AILMENT, AIL_SLEEP, 0, 0, 0, 0, 0, 0, 0, 2, 4, ST_NONE, 0, ST_NONE, 0, 0 },  // 147 spore
  { "Flash", "Destello", T_NORMAL, 0, 70, 20, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_ACCURACY, -1, ST_NONE, 0, 0 },  // 148 flash
  { "Psywave", "Psicoonda", T_PSYCHIC, 0, 80, 15, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 149 psywave
  { "Splash", "Salpicadura", T_NORMAL, 0, 0, 40, 0, MC_UNIQUE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 150 splash
  { "Acid Armor", "Armadura Acida", T_POISON, 0, 0, 40, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_DEFENSE, 2, ST_NONE, 0, 1 },  // 151 acid-armor
  { "Crabhammer", "Martillazo", T_WATER, 90, 85, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 1, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 152 crabhammer
  { "Explosion", "Explosi\242n", T_NORMAL, 170, 100, 5, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 153 explosion
  { "Fury Swipes", "Golpes Furia", T_NORMAL, 18, 80, 15, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 2, 5, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 154 fury-swipes
  { "Bonemerang", "Huesomerang", T_GROUND, 50, 90, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 2, 2, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 155 bonemerang
  { "Rest", "Descanso", T_PSYCHIC, 0, 0, 5, 0, MC_UNIQUE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 156 rest
  { "Rock Slide", "Avalancha", T_ROCK, 75, 90, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 157 rock-slide
  { "Hyper Fang", "Hipercolmillo", T_NORMAL, 80, 90, 15, 0, MC_DAMAGE, AIL_NONE, 10, 10, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 158 hyper-fang
  { "Sharpen", "Afilar", T_NORMAL, 0, 0, 30, 0, MC_NET_GOOD_STATS, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_ATTACK, 1, ST_NONE, 0, 1 },  // 159 sharpen
  { "Conversion", "Conversi\242n", T_NORMAL, 0, 0, 30, 0, MC_UNIQUE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 160 conversion
  { "Tri Attack", "Triataque", T_NORMAL, 80, 100, 10, 0, MC_DAMAGE_AILMENT, AIL_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 161 tri-attack
  { "Super Fang", "Superdiente", T_NORMAL, 0, 90, 10, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 162 super-fang
  { "Slash", "Cuchillada", T_NORMAL, 70, 100, 20, 0, MC_DAMAGE, AIL_NONE, 0, 0, 1, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 163 slash
  { "Substitute", "Sustituto", T_NORMAL, 0, 0, 10, 0, MC_UNIQUE, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 1 },  // 164 substitute
  { "Struggle", "Forcejeo", T_NORMAL, 50, 0, 1, 0, MC_DAMAGE, AIL_NONE, 0, 0, 0, 0, -25, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },  // 165 struggle
};

// ---- MT/MO de Rojo/Azul: 0..49 = MT01..MT50, 50..54 = MO01..MO05 ----
#define NUM_MACHINES 55
static const uint8_t MACHINE_MOVE[NUM_MACHINES] = {
    5,  13,  14,  18,  25,  92,  32,  34,  36,  38,  // TM01..TM10
   61,  55,  58,  59,  63,   6,  66,  68,  69,  99,  // TM11..TM20
   72,  76,  82,  85,  87,  89,  90,  91,  94, 100,  // TM21..TM30
  102, 104, 115, 117, 118, 120, 121, 126, 129, 130,  // TM31..TM40
  135, 138, 143, 156,  86, 149, 153, 157, 161, 164,  // TM41..TM50
   15,  19,  57,  70, 148,  // HM01..HM05
};

// ---- aprendizaje por nivel (Rojo/Azul); nivel 1 = movimientos iniciales ----
struct LearnEntry { uint8_t level, move; };
static const LearnEntry LEARN_TBL[] = {
  {1, 33}, {1, 45}, {7, 73}, {13, 22}, {20, 77}, {27, 75}, {34, 74}, {41, 79}, {48, 76},  // 1 bulbasaur
  {1, 33}, {1, 45}, {1, 73}, {13, 22}, {22, 77}, {30, 75}, {38, 74}, {46, 79}, {54, 76},  // 2 ivysaur
  {1, 22}, {1, 33}, {1, 45}, {1, 73}, {22, 77}, {30, 75}, {43, 74}, {55, 79}, {65, 76},  // 3 venusaur
  {1, 10}, {1, 45}, {9, 52}, {15, 43}, {22, 99}, {30, 163}, {38, 53}, {46, 83},  // 4 charmander
  {1, 10}, {1, 45}, {1, 52}, {15, 43}, {24, 99}, {33, 163}, {42, 53}, {56, 83},  // 5 charmeleon
  {1, 10}, {1, 43}, {1, 45}, {1, 52}, {24, 99}, {36, 163}, {46, 53}, {55, 83},  // 6 charizard
  {1, 33}, {1, 39}, {8, 145}, {15, 55}, {22, 44}, {28, 110}, {35, 130}, {42, 56},  // 7 squirtle
  {1, 33}, {1, 39}, {1, 145}, {15, 55}, {24, 44}, {31, 110}, {39, 130}, {47, 56},  // 8 wartortle
  {1, 33}, {1, 39}, {1, 55}, {1, 145}, {24, 44}, {31, 110}, {42, 130}, {52, 56},  // 9 blastoise
  {1, 33}, {1, 81},  // 10 caterpie
  {1, 106},  // 11 metapod
  {1, 93}, {15, 77}, {16, 78}, {17, 79}, {21, 48}, {26, 18}, {32, 60},  // 12 butterfree
  {1, 40}, {1, 81},  // 13 weedle
  {1, 106},  // 14 kakuna
  {1, 31}, {16, 116}, {20, 41}, {25, 99}, {30, 42}, {35, 97},  // 15 beedrill
  {1, 16}, {5, 28}, {12, 98}, {19, 18}, {28, 17}, {36, 97}, {44, 119},  // 16 pidgey
  {1, 16}, {1, 28}, {12, 98}, {21, 18}, {31, 17}, {40, 97}, {49, 119},  // 17 pidgeotto
  {1, 16}, {1, 28}, {1, 98}, {21, 18}, {31, 17}, {44, 97}, {54, 119},  // 18 pidgeot
  {1, 33}, {1, 39}, {7, 98}, {14, 158}, {23, 116}, {34, 162},  // 19 rattata
  {1, 33}, {1, 39}, {1, 98}, {14, 158}, {27, 116}, {41, 162},  // 20 raticate
  {1, 45}, {1, 64}, {9, 43}, {15, 31}, {22, 119}, {29, 65}, {36, 97},  // 21 spearow
  {1, 43}, {1, 45}, {1, 64}, {15, 31}, {25, 119}, {34, 65}, {43, 97},  // 22 fearow
  {1, 35}, {1, 43}, {10, 40}, {17, 44}, {24, 137}, {31, 103}, {38, 51},  // 23 ekans
  {1, 35}, {1, 40}, {1, 43}, {17, 44}, {27, 137}, {36, 103}, {47, 51},  // 24 arbok
  {1, 45}, {1, 84}, {9, 86}, {16, 98}, {26, 129}, {33, 97}, {43, 87},  // 25 pikachu
  {1, 45}, {1, 84}, {1, 86},  // 26 raichu
  {1, 10}, {10, 28}, {17, 163}, {24, 40}, {31, 129}, {38, 154},  // 27 sandshrew
  {1, 10}, {1, 28}, {17, 163}, {27, 40}, {36, 129}, {47, 154},  // 28 sandslash
  {1, 33}, {1, 45}, {8, 10}, {14, 40}, {21, 39}, {29, 44}, {36, 154}, {43, 24},  // 29 nidoran-f
  {1, 10}, {1, 33}, {1, 45}, {14, 40}, {23, 39}, {32, 44}, {41, 154}, {50, 24},  // 30 nidorina
  {1, 10}, {1, 33}, {1, 34}, {1, 39}, {14, 40},  // 31 nidoqueen
  {1, 33}, {1, 43}, {8, 30}, {14, 40}, {21, 116}, {29, 31}, {36, 32}, {43, 24},  // 32 nidoran-m
  {1, 30}, {1, 33}, {1, 43}, {14, 40}, {23, 116}, {32, 31}, {41, 32}, {50, 24},  // 33 nidorino
  {1, 30}, {1, 33}, {1, 37}, {1, 40},  // 34 nidoking
  {1, 1}, {1, 45}, {13, 47}, {18, 3}, {24, 107}, {31, 118}, {39, 111}, {48, 113},  // 35 clefairy
  {1, 3}, {1, 47}, {1, 107}, {1, 118},  // 36 clefable
  {1, 39}, {1, 52}, {16, 98}, {21, 46}, {28, 109}, {35, 53}, {42, 83},  // 37 vulpix
  {1, 39}, {1, 46}, {1, 52}, {1, 98},  // 38 ninetales
  {1, 47}, {9, 1}, {14, 50}, {19, 111}, {24, 3}, {29, 156}, {34, 34}, {39, 38},  // 39 jigglypuff
  {1, 3}, {1, 47}, {1, 50}, {1, 111},  // 40 wigglytuff
  {1, 141}, {10, 48}, {15, 44}, {21, 109}, {28, 17}, {36, 114},  // 41 zubat
  {1, 44}, {1, 103}, {1, 141}, {10, 48}, {21, 109}, {32, 17}, {43, 114},  // 42 golbat
  {1, 71}, {15, 77}, {17, 78}, {19, 79}, {24, 51}, {33, 80}, {46, 76},  // 43 oddish
  {1, 71}, {1, 77}, {1, 78}, {19, 79}, {28, 51}, {38, 80}, {52, 76},  // 44 gloom
  {1, 51}, {1, 78}, {1, 79}, {1, 80}, {15, 77},  // 45 vileplume
  {1, 10}, {13, 78}, {20, 141}, {27, 147}, {34, 163}, {41, 74},  // 46 paras
  {1, 10}, {1, 78}, {1, 141}, {30, 147}, {39, 163}, {48, 74},  // 47 parasect
  {1, 33}, {1, 50}, {24, 77}, {27, 141}, {30, 78}, {35, 60}, {38, 79}, {43, 94},  // 48 venonat
  {1, 33}, {1, 50}, {1, 77}, {1, 141}, {30, 78}, {38, 60}, {43, 79}, {50, 94},  // 49 venomoth
  {1, 10}, {15, 45}, {19, 91}, {24, 28}, {31, 163}, {40, 89},  // 50 diglett
  {1, 10}, {1, 45}, {1, 91}, {24, 28}, {35, 163}, {47, 89},  // 51 dugtrio
  {1, 10}, {1, 45}, {12, 44}, {17, 6}, {24, 103}, {33, 154}, {44, 163},  // 52 meowth
  {1, 10}, {1, 44}, {1, 45}, {1, 103}, {17, 6}, {37, 154}, {51, 163},  // 53 persian
  {1, 10}, {28, 39}, {31, 50}, {36, 93}, {43, 154}, {52, 56},  // 54 psyduck
  {1, 10}, {1, 39}, {1, 50}, {39, 93}, {48, 154}, {59, 56},  // 55 golduck
  {1, 10}, {1, 43}, {15, 2}, {21, 154}, {27, 116}, {33, 69}, {39, 37},  // 56 mankey
  {1, 2}, {1, 10}, {1, 43}, {1, 154}, {27, 116}, {37, 69}, {46, 37},  // 57 primeape
  {1, 44}, {1, 46}, {18, 52}, {23, 43}, {30, 36}, {39, 97}, {50, 53},  // 58 growlithe
  {1, 36}, {1, 43}, {1, 46}, {1, 52},  // 59 arcanine
  {1, 145}, {16, 95}, {19, 55}, {25, 3}, {31, 34}, {38, 133}, {45, 56},  // 60 poliwag
  {1, 55}, {1, 95}, {1, 145}, {26, 3}, {33, 34}, {41, 133}, {49, 56},  // 61 poliwhirl
  {1, 3}, {1, 34}, {1, 55}, {1, 95},  // 62 poliwrath
  {1, 100},  // 63 abra
  {1, 50}, {1, 93}, {1, 100}, {27, 60}, {31, 105}, {38, 94}, {42, 115},  // 64 kadabra
  {1, 50}, {1, 93}, {1, 100}, {27, 60}, {31, 105}, {38, 94}, {42, 115},  // 65 alakazam
  {1, 2}, {20, 67}, {25, 43}, {32, 116}, {39, 69}, {46, 66},  // 66 machop
  {1, 2}, {1, 43}, {1, 67}, {36, 116}, {44, 69}, {52, 66},  // 67 machoke
  {1, 2}, {1, 43}, {1, 67}, {36, 116}, {44, 69}, {52, 66},  // 68 machamp
  {1, 22}, {1, 74}, {13, 35}, {15, 77}, {18, 79}, {21, 78}, {26, 51}, {33, 75}, {42, 21},  // 69 bellsprout
  {1, 22}, {1, 35}, {1, 74}, {15, 77}, {18, 79}, {23, 78}, {29, 51}, {38, 75}, {49, 21},  // 70 weepinbell
  {1, 51}, {1, 75}, {1, 78}, {1, 79}, {13, 35}, {15, 77},  // 71 victreebel
  {1, 51}, {7, 48}, {13, 35}, {18, 40}, {22, 55}, {27, 132}, {33, 112}, {40, 103}, {48, 56},  // 72 tentacool
  {1, 35}, {1, 48}, {1, 51}, {18, 40}, {22, 55}, {27, 132}, {35, 112}, {43, 103}, {50, 56},  // 73 tentacruel
  {1, 33}, {11, 111}, {16, 88}, {21, 120}, {26, 106}, {31, 89}, {36, 153},  // 74 geodude
  {1, 33}, {1, 111}, {16, 88}, {21, 120}, {29, 106}, {36, 89}, {43, 153},  // 75 graveler
  {1, 33}, {1, 111}, {16, 88}, {21, 120}, {29, 106}, {36, 89}, {43, 153},  // 76 golem
  {1, 52}, {30, 39}, {32, 23}, {35, 45}, {39, 83}, {43, 36}, {48, 97},  // 77 ponyta
  {1, 23}, {1, 39}, {1, 45}, {1, 52}, {39, 83}, {47, 36}, {55, 97},  // 78 rapidash
  {1, 93}, {18, 50}, {22, 29}, {27, 45}, {33, 55}, {40, 133}, {48, 94},  // 79 slowpoke
  {1, 29}, {1, 50}, {1, 93}, {27, 45}, {33, 55}, {37, 110}, {44, 133}, {55, 94},  // 80 slowbro
  {1, 33}, {21, 49}, {25, 84}, {29, 48}, {35, 86}, {41, 129}, {47, 103},  // 81 magnemite
  {1, 33}, {1, 49}, {1, 84}, {29, 48}, {38, 86}, {46, 129}, {54, 103},  // 82 magneton
  {1, 28}, {1, 64}, {7, 43}, {15, 31}, {23, 14}, {31, 97}, {39, 163},  // 83 farfetchd
  {1, 64}, {20, 45}, {24, 31}, {30, 65}, {36, 99}, {40, 161}, {44, 97},  // 84 doduo
  {1, 31}, {1, 45}, {1, 64}, {30, 65}, {39, 99}, {45, 161}, {51, 97},  // 85 dodrio
  {1, 29}, {30, 45}, {35, 62}, {40, 156}, {45, 36}, {50, 58},  // 86 seel
  {1, 29}, {1, 45}, {1, 62}, {44, 156}, {50, 36}, {56, 58},  // 87 dewgong
  {1, 1}, {1, 50}, {30, 139}, {33, 107}, {37, 124}, {42, 106}, {48, 103}, {55, 151},  // 88 grimer
  {1, 1}, {1, 50}, {1, 139}, {33, 107}, {37, 124}, {45, 106}, {53, 103}, {60, 151},  // 89 muk
  {1, 33}, {1, 110}, {18, 48}, {23, 128}, {30, 62}, {39, 43}, {50, 58},  // 90 shellder
  {1, 48}, {1, 62}, {1, 110}, {1, 128}, {50, 131},  // 91 cloyster
  {1, 101}, {1, 109}, {1, 122}, {27, 95}, {35, 138},  // 92 gastly
  {1, 101}, {1, 109}, {1, 122}, {29, 95}, {38, 138},  // 93 haunter
  {1, 101}, {1, 109}, {1, 122}, {29, 95}, {38, 138},  // 94 gengar
  {1, 33}, {1, 103}, {15, 20}, {19, 88}, {25, 99}, {33, 21}, {43, 106},  // 95 onix
  {1, 1}, {1, 95}, {12, 50}, {17, 93}, {24, 29}, {29, 139}, {32, 94}, {37, 96},  // 96 drowzee
  {1, 1}, {1, 50}, {1, 93}, {1, 95}, {24, 29}, {33, 139}, {37, 94}, {43, 96},  // 97 hypno
  {1, 43}, {1, 145}, {20, 11}, {25, 12}, {30, 23}, {35, 152}, {40, 106},  // 98 krabby
  {1, 11}, {1, 43}, {1, 145}, {25, 12}, {34, 23}, {42, 152}, {49, 106},  // 99 kingler
  {1, 33}, {1, 103}, {17, 49}, {22, 120}, {29, 113}, {36, 129}, {43, 153},  // 100 voltorb
  {1, 33}, {1, 49}, {1, 103}, {22, 120}, {29, 113}, {40, 129}, {50, 153},  // 101 electrode
  {1, 95}, {1, 140}, {25, 115}, {28, 73}, {32, 78}, {37, 77}, {42, 76}, {48, 79},  // 102 exeggcute
  {1, 95}, {1, 140}, {28, 23},  // 103 exeggutor
  {1, 45}, {1, 125}, {25, 43}, {31, 116}, {38, 37}, {43, 155}, {46, 99},  // 104 cubone
  {1, 43}, {1, 45}, {1, 116}, {1, 125}, {41, 37}, {48, 155}, {55, 99},  // 105 marowak
  {1, 24}, {1, 96}, {33, 27}, {38, 26}, {43, 116}, {48, 136}, {53, 25},  // 106 hitmonlee
  {1, 4}, {1, 97}, {33, 7}, {38, 8}, {43, 9}, {48, 5}, {53, 68},  // 107 hitmonchan
  {1, 35}, {1, 48}, {7, 23}, {15, 50}, {23, 111}, {31, 21}, {39, 103},  // 108 lickitung
  {1, 33}, {1, 123}, {32, 124}, {37, 108}, {40, 120}, {45, 114}, {48, 153},  // 109 koffing
  {1, 33}, {1, 123}, {1, 124}, {39, 108}, {43, 120}, {49, 114}, {53, 153},  // 110 weezing
  {1, 30}, {30, 23}, {35, 39}, {40, 31}, {45, 32}, {50, 43}, {55, 36},  // 111 rhyhorn
  {1, 23}, {1, 30}, {1, 31}, {1, 39}, {48, 32}, {55, 43}, {64, 36},  // 112 rhydon
  {1, 1}, {1, 3}, {24, 47}, {30, 45}, {38, 107}, {44, 111}, {48, 113}, {54, 38},  // 113 chansey
  {1, 20}, {1, 132}, {29, 71}, {32, 77}, {36, 78}, {39, 79}, {45, 21}, {49, 74},  // 114 tangela
  {1, 4}, {1, 99}, {26, 44}, {31, 39}, {36, 5}, {41, 43}, {46, 146},  // 115 kangaskhan
  {1, 145}, {19, 108}, {24, 43}, {30, 55}, {37, 97}, {45, 56},  // 116 horsea
  {1, 108}, {1, 145}, {24, 43}, {30, 55}, {41, 97}, {52, 56},  // 117 seadra
  {1, 39}, {1, 64}, {19, 48}, {24, 30}, {30, 31}, {37, 127}, {45, 32}, {54, 97},  // 118 goldeen
  {1, 39}, {1, 48}, {1, 64}, {24, 30}, {30, 31}, {39, 127}, {48, 32}, {54, 97},  // 119 seaking
  {1, 33}, {17, 55}, {22, 106}, {27, 105}, {32, 129}, {37, 107}, {42, 113}, {47, 56},  // 120 staryu
  {1, 33}, {1, 55}, {1, 106},  // 121 starmie
  {1, 93}, {1, 112}, {23, 113}, {31, 3}, {39, 96}, {47, 164},  // 122 mr-mime
  {1, 98}, {17, 43}, {20, 116}, {24, 104}, {29, 163}, {35, 14}, {42, 97},  // 123 scyther
  {1, 1}, {1, 142}, {18, 122}, {23, 3}, {31, 8}, {39, 34}, {47, 37}, {58, 59},  // 124 jynx
  {1, 43}, {1, 98}, {34, 84}, {37, 103}, {42, 9}, {49, 113}, {54, 87},  // 125 electabuzz
  {1, 52}, {36, 43}, {39, 109}, {43, 7}, {48, 108}, {52, 123}, {55, 53},  // 126 magmar
  {1, 11}, {25, 69}, {30, 12}, {36, 116}, {43, 106}, {49, 163}, {54, 14},  // 127 pinsir
  {1, 33}, {21, 23}, {28, 39}, {35, 43}, {44, 99}, {51, 36},  // 128 tauros
  {1, 150}, {15, 33},  // 129 magikarp
  {1, 43}, {1, 44}, {1, 56}, {1, 82}, {52, 63},  // 130 gyarados
  {1, 45}, {1, 55}, {16, 47}, {20, 54}, {25, 34}, {31, 109}, {38, 58}, {46, 56},  // 131 lapras
  {1, 144},  // 132 ditto
  {1, 28}, {1, 33}, {27, 98}, {31, 39}, {37, 44}, {45, 36},  // 133 eevee
  {1, 28}, {1, 33}, {1, 55}, {1, 98}, {37, 39}, {40, 44}, {42, 151}, {44, 114}, {48, 54}, {54, 56},  // 134 vaporeon
  {1, 28}, {1, 33}, {1, 84}, {1, 98}, {37, 39}, {40, 86}, {42, 24}, {44, 97}, {48, 42}, {54, 87},  // 135 jolteon
  {1, 28}, {1, 33}, {1, 52}, {1, 98}, {37, 39}, {40, 44}, {42, 43}, {44, 83}, {48, 99}, {54, 53},  // 136 flareon
  {1, 33}, {1, 159}, {1, 160}, {23, 60}, {28, 105}, {35, 97}, {42, 161},  // 137 porygon
  {1, 55}, {1, 110}, {34, 30}, {39, 43}, {46, 131}, {53, 56},  // 138 omanyte
  {1, 30}, {1, 55}, {1, 110}, {39, 43}, {44, 131}, {49, 56},  // 139 omastar
  {1, 10}, {1, 106}, {34, 71}, {39, 163}, {44, 43}, {49, 56},  // 140 kabuto
  {1, 10}, {1, 71}, {1, 106}, {39, 163}, {46, 43}, {53, 56},  // 141 kabutops
  {1, 17}, {1, 97}, {33, 48}, {38, 44}, {45, 36}, {54, 63},  // 142 aerodactyl
  {1, 29}, {1, 133}, {1, 156}, {35, 34}, {41, 106}, {48, 38}, {56, 63},  // 143 snorlax
  {1, 58}, {1, 64}, {51, 59}, {55, 97}, {60, 54},  // 144 articuno
  {1, 65}, {1, 84}, {51, 87}, {55, 97}, {60, 113},  // 145 zapdos
  {1, 64}, {1, 83}, {51, 43}, {55, 97}, {60, 143},  // 146 moltres
  {1, 35}, {1, 43}, {10, 86}, {20, 97}, {30, 21}, {40, 82}, {50, 63},  // 147 dratini
  {1, 35}, {1, 43}, {1, 86}, {20, 97}, {35, 21}, {45, 82}, {55, 63},  // 148 dragonair
  {1, 35}, {1, 43}, {1, 86}, {1, 97}, {35, 21}, {45, 82}, {60, 63},  // 149 dragonite
  {1, 50}, {1, 93}, {1, 94}, {1, 129}, {63, 112}, {70, 105}, {75, 54}, {81, 133},  // 150 mewtwo
  {1, 1}, {10, 144}, {20, 5}, {30, 118}, {40, 94},  // 151 mew
};

// ---- especies: indice = numero de Pokedex, 0 sin usar ----
struct BattleSpecies {
  uint8_t type1, type2;          // type2 = T_NONE si es de un tipo
  uint8_t hp, atk, def, spe, spc;  // base stats de gen 1 (Special unico)
  uint8_t captureRate;           // 3..255
  uint16_t baseExp;              // la actual de PokeAPI
  uint8_t growth;                // Growth
  uint8_t flee;                  // % de huida tras un lanzamiento fallido
  uint16_t learnStart; uint8_t learnCount;  // tramo en LEARN_TBL
  uint64_t machines;             // bit i = puede aprender MACHINE_MOVE[i]
};

static const BattleSpecies BSPECIES[152] = {
  { T_NORMAL, T_NONE, 0, 0, 0, 0, 0, 0, 0, G_MEDIUM, 0, 0, 0, 0ULL },
  { T_GRASS, T_POISON, 45, 49, 49, 45, 65, 45, 64, G_MEDIUM_SLOW, 21, 0, 9, 0x060803C03803A4ULL },  // 1 bulbasaur
  { T_GRASS, T_POISON, 60, 62, 63, 60, 80, 45, 142, G_MEDIUM_SLOW, 21, 9, 9, 0x060803C03803A4ULL },  // 2 ivysaur
  { T_GRASS, T_POISON, 80, 82, 83, 80, 100, 45, 236, G_MEDIUM_SLOW, 21, 18, 9, 0x060803C03843A4ULL },  // 3 venusaur
  { T_FIRE, T_NONE, 39, 52, 43, 65, 50, 45, 62, G_MEDIUM_SLOW, 21, 27, 8, 0x2608E3C84F03B5ULL },  // 4 charmander
  { T_FIRE, T_NONE, 58, 64, 58, 80, 65, 45, 142, G_MEDIUM_SLOW, 21, 35, 8, 0x2608E3C84F03B5ULL },  // 5 charmeleon
  { T_FIRE, T_FLYING, 78, 84, 78, 100, 85, 45, 240, G_MEDIUM_SLOW, 21, 43, 8, 0x2608E3CE4F43B5ULL },  // 6 charizard
  { T_WATER, T_NONE, 44, 48, 65, 43, 50, 45, 63, G_MEDIUM_SLOW, 21, 51, 8, 0x320883C80F3FB1ULL },  // 7 squirtle
  { T_WATER, T_NONE, 59, 63, 80, 58, 65, 45, 142, G_MEDIUM_SLOW, 21, 59, 8, 0x320883C80F3FB1ULL },  // 8 wartortle
  { T_WATER, T_NONE, 79, 83, 100, 78, 85, 45, 239, G_MEDIUM_SLOW, 21, 67, 8, 0x320883CE0F7FB1ULL },  // 9 blastoise
  { T_BUG, T_NONE, 45, 30, 35, 45, 20, 255, 39, G_MEDIUM, 5, 75, 2, 0x00000000000000ULL },  // 10 caterpie
  { T_BUG, T_NONE, 50, 20, 55, 30, 25, 120, 72, G_MEDIUM, 15, 77, 1, 0x00000000000000ULL },  // 11 metapod
  { T_BUG, T_FLYING, 60, 45, 50, 70, 80, 45, 178, G_MEDIUM, 21, 78, 7, 0x022843F038432AULL },  // 12 butterfree
  { T_BUG, T_POISON, 40, 35, 30, 50, 20, 255, 39, G_MEDIUM, 5, 85, 2, 0x00000000000000ULL },  // 13 weedle
  { T_BUG, T_POISON, 45, 25, 50, 35, 25, 120, 72, G_MEDIUM, 15, 87, 1, 0x00000000000000ULL },  // 14 kakuna
  { T_BUG, T_POISON, 65, 90, 40, 75, 45, 45, 178, G_MEDIUM, 21, 88, 6, 0x0608C3C0184324ULL },  // 15 beedrill
  { T_NORMAL, T_FLYING, 40, 45, 40, 56, 35, 255, 50, G_MEDIUM_SLOW, 5, 94, 7, 0x0A0C43C008032AULL },  // 16 pidgey
  { T_NORMAL, T_FLYING, 63, 60, 55, 71, 50, 120, 122, G_MEDIUM_SLOW, 15, 101, 7, 0x0A0C43C008032AULL },  // 17 pidgeotto
  { T_NORMAL, T_FLYING, 83, 80, 75, 101, 70, 45, 216, G_MEDIUM_SLOW, 21, 108, 7, 0x0A0C43C008432AULL },  // 18 pidgeot
  { T_NORMAL, T_NONE, 30, 56, 35, 72, 25, 255, 51, G_MEDIUM, 5, 115, 6, 0x0208C2C9882FA0ULL },  // 19 rattata
  { T_NORMAL, T_NONE, 55, 81, 60, 97, 50, 127, 145, G_MEDIUM, 15, 121, 6, 0x0208C2C9887FA0ULL },  // 20 raticate
  { T_NORMAL, T_FLYING, 40, 60, 30, 70, 31, 255, 52, G_MEDIUM, 5, 127, 7, 0x0A0C42C008032AULL },  // 21 spearow
  { T_NORMAL, T_FLYING, 65, 90, 65, 100, 61, 90, 155, G_MEDIUM, 18, 134, 7, 0x0A0C42C008432AULL },  // 22 fearow
  { T_POISON, T_NONE, 35, 60, 44, 55, 40, 255, 58, G_MEDIUM, 5, 141, 7, 0x228882CE1803A0ULL },  // 23 ekans
  { T_POISON, T_NONE, 60, 95, 69, 80, 65, 90, 157, G_MEDIUM, 18, 148, 7, 0x228882CE1843A0ULL },  // 24 arbok
  { T_ELECTRIC, T_NONE, 35, 55, 40, 90, 50, 190, 112, G_MEDIUM, 10, 155, 7, 0x4218C3C18D83B1ULL },  // 25 pikachu
  { T_ELECTRIC, T_NONE, 60, 90, 55, 110, 90, 75, 218, G_MEDIUM, 19, 162, 3, 0x4218C3C18DC3B1ULL },  // 26 raichu
  { T_GROUND, T_NONE, 50, 75, 85, 40, 30, 255, 60, G_MEDIUM, 5, 165, 6, 0x2688C2CE0D03A4ULL },  // 27 sandshrew
  { T_GROUND, T_NONE, 75, 100, 110, 65, 55, 90, 158, G_MEDIUM, 18, 171, 6, 0x2688C2CE0D43A4ULL },  // 28 sandslash
  { T_POISON, T_NONE, 55, 47, 52, 41, 40, 235, 55, G_MEDIUM_SLOW, 6, 177, 8, 0x020883C18823A0ULL },  // 29 nidoran-f
  { T_POISON, T_NONE, 70, 62, 67, 56, 55, 120, 128, G_MEDIUM_SLOW, 15, 185, 8, 0x020883C1883FE0ULL },  // 30 nidorina
  { T_POISON, T_GROUND, 90, 92, 87, 76, 75, 45, 227, G_MEDIUM_SLOW, 21, 193, 5, 0x3288A3C78FFFF1ULL },  // 31 nidoqueen
  { T_POISON, T_NONE, 46, 57, 40, 50, 40, 235, 55, G_MEDIUM_SLOW, 6, 198, 8, 0x020883C18823E0ULL },  // 32 nidoran-m
  { T_POISON, T_NONE, 61, 72, 57, 65, 55, 120, 128, G_MEDIUM_SLOW, 15, 206, 8, 0x020883C1883FE0ULL },  // 33 nidorino
  { T_POISON, T_GROUND, 81, 102, 77, 85, 75, 45, 227, G_MEDIUM_SLOW, 21, 214, 4, 0x3288A3C78FFFF1ULL },  // 34 nidoking
  { T_NORMAL, T_NONE, 70, 45, 48, 35, 60, 150, 113, G_FAST, 13, 218, 8, 0x6338A7F1AF3FB1ULL },  // 35 clefairy
  { T_NORMAL, T_NONE, 95, 70, 73, 60, 85, 25, 217, G_FAST, 23, 226, 4, 0x6338A7F1AF7FB1ULL },  // 36 clefable
  { T_FIRE, T_NONE, 38, 41, 40, 65, 65, 190, 60, G_MEDIUM, 10, 230, 7, 0x0208E3C80803A0ULL },  // 37 vulpix
  { T_FIRE, T_NONE, 73, 76, 75, 100, 100, 75, 177, G_MEDIUM, 19, 237, 4, 0x0208E3C80843A0ULL },  // 38 ninetales
  { T_NORMAL, T_NONE, 115, 45, 20, 20, 25, 170, 95, G_FAST, 11, 241, 8, 0x6338A3F1AF3FB1ULL },  // 39 jigglypuff
  { T_NORMAL, T_NONE, 140, 70, 45, 45, 50, 50, 196, G_FAST, 21, 249, 4, 0x6338A3F1AF7FB1ULL },  // 40 wigglytuff
  { T_POISON, T_FLYING, 40, 45, 35, 55, 40, 255, 49, G_MEDIUM, 5, 253, 6, 0x020842C018032AULL },  // 41 zubat
  { T_POISON, T_FLYING, 75, 80, 70, 90, 75, 90, 159, G_MEDIUM, 18, 259, 7, 0x020842C018432AULL },  // 42 golbat
  { T_GRASS, T_POISON, 45, 50, 55, 30, 75, 255, 64, G_MEDIUM_SLOW, 5, 266, 7, 0x060803C0380324ULL },  // 43 oddish
  { T_GRASS, T_POISON, 60, 65, 70, 40, 85, 120, 138, G_MEDIUM_SLOW, 15, 273, 7, 0x060803C0380324ULL },  // 44 gloom
  { T_GRASS, T_POISON, 75, 80, 85, 50, 100, 45, 221, G_MEDIUM_SLOW, 21, 280, 5, 0x060803C03843A4ULL },  // 45 vileplume
  { T_BUG, T_GRASS, 35, 70, 55, 25, 55, 190, 57, G_MEDIUM, 10, 285, 6, 0x060883C83803A4ULL },  // 46 paras
  { T_BUG, T_GRASS, 60, 95, 80, 30, 80, 75, 142, G_MEDIUM, 19, 291, 6, 0x060883C83843A4ULL },  // 47 parasect
  { T_BUG, T_POISON, 60, 55, 50, 45, 40, 190, 61, G_MEDIUM, 10, 297, 8, 0x022803D0380320ULL },  // 48 venonat
  { T_BUG, T_POISON, 70, 65, 60, 90, 90, 75, 158, G_MEDIUM, 19, 305, 8, 0x022843F038432AULL },  // 49 venomoth
  { T_GROUND, T_NONE, 10, 55, 25, 95, 45, 255, 53, G_MEDIUM, 5, 313, 6, 0x028802CE0803A0ULL },  // 50 diglett
  { T_GROUND, T_NONE, 35, 100, 50, 120, 70, 50, 149, G_MEDIUM, 21, 319, 6, 0x028802CE0843A0ULL },  // 51 dugtrio
  { T_NORMAL, T_NONE, 40, 45, 35, 90, 40, 255, 58, G_MEDIUM, 5, 325, 7, 0x0208C2C1888FA0ULL },  // 52 meowth
  { T_NORMAL, T_NONE, 65, 70, 60, 115, 65, 90, 154, G_MEDIUM, 18, 332, 7, 0x0208C2C188CFA0ULL },  // 53 persian
  { T_WATER, T_NONE, 50, 52, 48, 55, 50, 190, 64, G_MEDIUM, 10, 339, 6, 0x3208C2C80FBFB1ULL },  // 54 psyduck
  { T_WATER, T_NONE, 80, 82, 78, 85, 80, 75, 175, G_MEDIUM, 19, 345, 6, 0x3208C2C80FFFB1ULL },  // 55 golduck
  { T_FIGHTING, T_NONE, 40, 80, 35, 70, 35, 190, 61, G_MEDIUM, 10, 351, 7, 0x2288C6C98F83B1ULL },  // 56 mankey
  { T_FIGHTING, T_NONE, 65, 105, 60, 95, 60, 75, 159, G_MEDIUM, 19, 358, 7, 0x2288C6C98FC3B1ULL },  // 57 primeape
  { T_FIRE, T_NONE, 55, 70, 45, 60, 50, 190, 70, G_SLOW, 10, 365, 7, 0x0208E3C84803A0ULL },  // 58 growlithe
  { T_FIRE, T_NONE, 90, 110, 80, 95, 80, 75, 194, G_SLOW, 19, 372, 4, 0x0208E3E84843A0ULL },  // 59 arcanine
  { T_WATER, T_NONE, 40, 50, 40, 90, 40, 255, 60, G_MEDIUM_SLOW, 5, 376, 7, 0x122882D0083FA0ULL },  // 60 poliwag
  { T_WATER, T_NONE, 65, 65, 65, 90, 50, 120, 135, G_MEDIUM_SLOW, 15, 383, 7, 0x322886D60F3FB1ULL },  // 61 poliwhirl
  { T_WATER, T_FIGHTING, 90, 95, 95, 70, 70, 45, 230, G_MEDIUM_SLOW, 21, 390, 4, 0x322886D60F7FB1ULL },  // 62 poliwrath
  { T_PSYCHIC, T_NONE, 25, 20, 15, 90, 105, 200, 62, G_MEDIUM_SLOW, 9, 394, 1, 0x433887F00F03B1ULL },  // 63 abra
  { T_PSYCHIC, T_NONE, 40, 35, 30, 105, 120, 100, 140, G_MEDIUM_SLOW, 17, 395, 7, 0x433887F80F03B1ULL },  // 64 kadabra
  { T_PSYCHIC, T_NONE, 55, 50, 45, 120, 135, 50, 225, G_MEDIUM_SLOW, 21, 402, 7, 0x433887F80F43B1ULL },  // 65 alakazam
  { T_FIGHTING, T_NONE, 70, 80, 50, 35, 35, 180, 61, G_MEDIUM_SLOW, 10, 409, 6, 0x2288A6CE0F03B1ULL },  // 66 machop
  { T_FIGHTING, T_NONE, 80, 100, 70, 45, 50, 90, 142, G_MEDIUM_SLOW, 18, 415, 6, 0x2288A6CE0F03B1ULL },  // 67 machoke
  { T_FIGHTING, T_NONE, 90, 130, 80, 55, 65, 45, 227, G_MEDIUM_SLOW, 21, 421, 6, 0x2288A6CE0F43B1ULL },  // 68 machamp
  { T_GRASS, T_POISON, 50, 75, 35, 40, 70, 255, 60, G_MEDIUM_SLOW, 5, 427, 9, 0x060803C0380324ULL },  // 69 bellsprout
  { T_GRASS, T_POISON, 65, 90, 50, 55, 85, 120, 137, G_MEDIUM_SLOW, 15, 436, 9, 0x060803C0380324ULL },  // 70 weepinbell
  { T_GRASS, T_POISON, 80, 105, 65, 70, 100, 45, 221, G_MEDIUM_SLOW, 21, 445, 6, 0x060803C03843A4ULL },  // 71 victreebel
  { T_WATER, T_POISON, 40, 40, 35, 70, 100, 190, 67, G_SLOW, 10, 451, 9, 0x160883C0183F24ULL },  // 72 tentacool
  { T_WATER, T_POISON, 80, 70, 65, 100, 120, 60, 180, G_SLOW, 20, 460, 9, 0x160883C0187F24ULL },  // 73 tentacruel
  { T_ROCK, T_GROUND, 40, 80, 100, 20, 30, 255, 60, G_MEDIUM_SLOW, 5, 469, 7, 0x22C82ECE0F03A1ULL },  // 74 geodude
  { T_ROCK, T_GROUND, 55, 95, 115, 35, 45, 120, 137, G_MEDIUM_SLOW, 15, 476, 7, 0x22C82ECE0F03A1ULL },  // 75 graveler
  { T_ROCK, T_GROUND, 80, 120, 130, 45, 55, 45, 223, G_MEDIUM_SLOW, 21, 483, 7, 0x22C82ECE0F43B1ULL },  // 76 golem
  { T_FIRE, T_NONE, 50, 85, 55, 90, 65, 190, 82, G_MEDIUM, 10, 490, 7, 0x0208E3C00803E0ULL },  // 77 ponyta
  { T_FIRE, T_NONE, 65, 100, 70, 105, 80, 60, 175, G_MEDIUM, 20, 497, 7, 0x0208E3C00843E0ULL },  // 78 rapidash
  { T_WATER, T_PSYCHIC, 90, 65, 65, 15, 40, 190, 63, G_MEDIUM, 10, 504, 7, 0x7338E3FE08BFA0ULL },  // 79 slowpoke
  { T_WATER, T_PSYCHIC, 95, 75, 110, 30, 80, 75, 172, G_MEDIUM, 19, 511, 8, 0x7338E3FE0FFFB1ULL },  // 80 slowbro
  { T_ELECTRIC, T_NONE, 25, 35, 70, 45, 95, 190, 65, G_MEDIUM, 10, 519, 7, 0x421843E1880320ULL },  // 81 magnemite
  { T_ELECTRIC, T_NONE, 50, 60, 95, 70, 120, 60, 163, G_MEDIUM, 20, 526, 7, 0x421843E1884320ULL },  // 82 magneton
  { T_NORMAL, T_FLYING, 52, 90, 55, 60, 58, 45, 132, G_MEDIUM, 21, 533, 7, 0x0E08C3C00803AEULL },  // 83 farfetchd
  { T_NORMAL, T_FLYING, 35, 85, 45, 75, 35, 190, 62, G_MEDIUM, 10, 540, 7, 0x0B0C83C00803A8ULL },  // 84 doduo
  { T_NORMAL, T_FLYING, 60, 110, 70, 110, 60, 45, 165, G_MEDIUM, 21, 547, 7, 0x0B0C83C00843A8ULL },  // 85 dodrio
  { T_WATER, T_NONE, 65, 45, 55, 45, 70, 190, 65, G_MEDIUM, 10, 554, 6, 0x320882C008BFE0ULL },  // 86 seel
  { T_WATER, T_ICE, 90, 70, 80, 70, 95, 75, 166, G_MEDIUM, 19, 560, 6, 0x320882C008FFE0ULL },  // 87 dewgong
  { T_POISON, T_NONE, 80, 80, 50, 25, 40, 190, 65, G_MEDIUM, 10, 566, 8, 0x02482AC19800A0ULL },  // 88 grimer
  { T_POISON, T_NONE, 105, 105, 75, 50, 65, 75, 175, G_MEDIUM, 19, 574, 8, 0x02482AC19840A0ULL },  // 89 muk
  { T_WATER, T_NONE, 30, 65, 100, 40, 45, 190, 61, G_SLOW, 10, 582, 7, 0x13484BE0083F20ULL },  // 90 shellder
  { T_WATER, T_ICE, 50, 95, 180, 70, 85, 60, 184, G_SLOW, 20, 589, 5, 0x13484BE0087F20ULL },  // 91 cloyster
  { T_GHOST, T_POISON, 30, 35, 30, 80, 100, 190, 62, G_MEDIUM_SLOW, 10, 594, 5, 0x026A0AD1980020ULL },  // 92 gastly
  { T_GHOST, T_POISON, 45, 50, 45, 95, 115, 90, 142, G_MEDIUM_SLOW, 18, 599, 5, 0x026A0AD1980020ULL },  // 93 haunter
  { T_GHOST, T_POISON, 60, 65, 60, 110, 130, 45, 225, G_MEDIUM_SLOW, 21, 604, 5, 0x226A8ED19F43B1ULL },  // 94 gengar
  { T_ROCK, T_GROUND, 35, 45, 160, 70, 30, 45, 77, G_MEDIUM, 21, 609, 7, 0x22C88ACE0803A0ULL },  // 95 onix
  { T_PSYCHIC, T_NONE, 60, 48, 45, 42, 90, 190, 66, G_MEDIUM, 10, 616, 8, 0x433A87F00F03B1ULL },  // 96 drowzee
  { T_PSYCHIC, T_NONE, 85, 73, 70, 67, 115, 75, 169, G_MEDIUM, 19, 624, 8, 0x433A87F00F43B1ULL },  // 97 hypno
  { T_WATER, T_NONE, 30, 105, 90, 50, 25, 225, 65, G_MEDIUM, 7, 632, 7, 0x360802C0083FA4ULL },  // 98 krabby
  { T_WATER, T_NONE, 55, 130, 115, 75, 50, 60, 166, G_MEDIUM, 20, 639, 7, 0x360802C0087FA4ULL },  // 99 kingler
  { T_ELECTRIC, T_NONE, 40, 30, 50, 100, 55, 190, 66, G_MEDIUM, 10, 646, 7, 0x42584BE1880120ULL },  // 100 voltorb
  { T_ELECTRIC, T_NONE, 60, 50, 70, 150, 80, 60, 172, G_MEDIUM, 20, 653, 7, 0x4258CBE1884120ULL },  // 101 electrode
  { T_GRASS, T_PSYCHIC, 60, 40, 80, 40, 60, 90, 65, G_SLOW, 18, 660, 8, 0x02681BF0080320ULL },  // 102 exeggcute
  { T_GRASS, T_PSYCHIC, 95, 95, 85, 55, 125, 45, 186, G_SLOW, 21, 668, 3, 0x22681BF0384320ULL },  // 103 exeggutor
  { T_GROUND, T_NONE, 50, 50, 95, 35, 40, 190, 64, G_MEDIUM, 10, 671, 7, 0x2208A2CE0F3FB1ULL },  // 104 cubone
  { T_GROUND, T_NONE, 60, 80, 110, 45, 50, 75, 149, G_MEDIUM, 19, 678, 7, 0x2208A2CE0F7FB1ULL },  // 105 marowak
  { T_FIGHTING, T_NONE, 50, 120, 53, 87, 35, 45, 159, G_MEDIUM, 21, 685, 7, 0x2208C6C00F03B1ULL },  // 106 hitmonlee
  { T_FIGHTING, T_NONE, 50, 105, 79, 76, 35, 45, 159, G_MEDIUM, 21, 692, 7, 0x2208C6C00F03B1ULL },  // 107 hitmonchan
  { T_NORMAL, T_NONE, 90, 55, 75, 30, 60, 45, 77, G_MEDIUM, 21, 699, 7, 0x3608A2C78F7FB5ULL },  // 108 lickitung
  { T_POISON, T_NONE, 40, 65, 95, 35, 60, 190, 68, G_MEDIUM, 10, 706, 7, 0x02482AC1880020ULL },  // 109 koffing
  { T_POISON, T_NONE, 65, 90, 120, 60, 85, 60, 172, G_MEDIUM, 20, 713, 7, 0x02482AC1884020ULL },  // 110 weezing
  { T_GROUND, T_ROCK, 80, 85, 95, 25, 30, 120, 69, G_SLOW, 15, 720, 7, 0x2288A2CF8803E0ULL },  // 111 rhyhorn
  { T_GROUND, T_ROCK, 105, 130, 120, 40, 45, 60, 170, G_SLOW, 20, 727, 7, 0x3288A2CF8FFFF1ULL },  // 112 rhydon
  { T_NORMAL, T_NONE, 250, 5, 5, 50, 105, 30, 395, G_FAST, 22, 734, 8, 0x6339B7F1AF7FB1ULL },  // 113 chansey
  { T_GRASS, T_NONE, 65, 55, 115, 60, 100, 45, 87, G_MEDIUM, 21, 742, 8, 0x060882C03843A4ULL },  // 114 tangela
  { T_NORMAL, T_NONE, 105, 95, 80, 90, 40, 45, 172, G_MEDIUM, 21, 750, 7, 0x3288A2C78F7FB1ULL },  // 115 kangaskhan
  { T_WATER, T_NONE, 30, 40, 70, 60, 70, 225, 59, G_MEDIUM, 7, 757, 6, 0x1208C2C0083F20ULL },  // 116 horsea
  { T_WATER, T_NONE, 55, 65, 95, 85, 95, 75, 154, G_MEDIUM, 19, 763, 6, 0x1208C2C0087F20ULL },  // 117 seadra
  { T_WATER, T_NONE, 45, 67, 60, 63, 50, 225, 64, G_MEDIUM, 7, 769, 8, 0x1208C2C0083F60ULL },  // 118 goldeen
  { T_WATER, T_NONE, 80, 92, 65, 68, 80, 60, 158, G_MEDIUM, 20, 777, 8, 0x1208C2C0087F60ULL },  // 119 seaking
  { T_WATER, T_NONE, 30, 45, 55, 85, 70, 225, 68, G_SLOW, 7, 785, 8, 0x5338C3F1883F20ULL },  // 120 staryu
  { T_WATER, T_PSYCHIC, 60, 75, 85, 115, 100, 60, 182, G_SLOW, 20, 793, 3, 0x5338C3F1887F20ULL },  // 121 starmie
  { T_PSYCHIC, T_NONE, 40, 45, 65, 90, 100, 45, 161, G_MEDIUM, 21, 796, 6, 0x423887F1AF43B1ULL },  // 122 mr-mime
  { T_BUG, T_FLYING, 70, 110, 80, 105, 55, 45, 100, G_MEDIUM, 21, 802, 7, 0x0608C2C0084324ULL },  // 123 scyther
  { T_ICE, T_PSYCHIC, 65, 50, 35, 95, 95, 45, 159, G_MEDIUM, 21, 809, 8, 0x022887F00F7FB1ULL },  // 124 jynx
  { T_ELECTRIC, T_NONE, 65, 83, 57, 105, 85, 45, 172, G_MEDIUM, 21, 817, 7, 0x6238C7F18F43B1ULL },  // 125 electabuzz
  { T_FIRE, T_NONE, 65, 95, 57, 93, 85, 45, 173, G_MEDIUM, 21, 824, 7, 0x2228A6F00F43B1ULL },  // 126 magmar
  { T_BUG, T_NONE, 65, 125, 100, 85, 55, 45, 175, G_SLOW, 21, 831, 7, 0x260802C00D43A4ULL },  // 127 pinsir
  { T_NORMAL, T_NONE, 75, 100, 95, 110, 70, 45, 172, G_SLOW, 21, 838, 6, 0x2208A2C78873E0ULL },  // 128 tauros
  { T_WATER, T_NONE, 20, 10, 55, 80, 20, 255, 40, G_SLOW, 5, 844, 2, 0x00000000000000ULL },  // 129 magikarp
  { T_WATER, T_FLYING, 95, 125, 79, 81, 100, 45, 189, G_SLOW, 21, 846, 5, 0x3208A3C1C87FA0ULL },  // 130 gyarados
  { T_WATER, T_ICE, 130, 85, 80, 60, 95, 45, 187, G_SLOW, 21, 851, 8, 0x322883D1E87FE0ULL },  // 131 lapras
  { T_NORMAL, T_NONE, 48, 48, 48, 48, 48, 35, 101, G_MEDIUM, 22, 859, 1, 0x00000000000000ULL },  // 132 ditto
  { T_NORMAL, T_NONE, 55, 55, 50, 55, 65, 45, 65, G_MEDIUM, 21, 860, 6, 0x0208C3C00803A0ULL },  // 133 eevee
  { T_WATER, T_NONE, 130, 65, 60, 65, 110, 45, 184, G_MEDIUM, 21, 866, 10, 0x1208C3C0087FA0ULL },  // 134 vaporeon
  { T_ELECTRIC, T_NONE, 65, 65, 60, 130, 110, 45, 184, G_MEDIUM, 21, 876, 10, 0x4218C3C18843A0ULL },  // 135 jolteon
  { T_FIRE, T_NONE, 65, 130, 60, 65, 110, 45, 184, G_MEDIUM, 21, 886, 10, 0x0208E3C00843A0ULL },  // 136 flareon
  { T_NORMAL, T_NONE, 65, 60, 70, 40, 75, 45, 79, G_MEDIUM, 21, 896, 7, 0x4338C3F1887320ULL },  // 137 porygon
  { T_ROCK, T_WATER, 35, 40, 100, 35, 90, 45, 71, G_MEDIUM, 21, 903, 6, 0x120803C0083FA0ULL },  // 138 omanyte
  { T_ROCK, T_WATER, 70, 60, 125, 55, 115, 45, 173, G_MEDIUM, 21, 909, 6, 0x120883C00D7FE0ULL },  // 139 omastar
  { T_ROCK, T_WATER, 30, 80, 90, 55, 45, 45, 71, G_MEDIUM, 21, 915, 6, 0x120803C0083FA0ULL },  // 140 kabuto
  { T_ROCK, T_WATER, 60, 115, 105, 80, 70, 45, 173, G_MEDIUM, 21, 921, 6, 0x120883C00D7FB6ULL },  // 141 kabutops
  { T_ROCK, T_FLYING, 80, 105, 65, 130, 60, 45, 180, G_SLOW, 21, 927, 6, 0x0A0C63C048432AULL },  // 142 aerodactyl
  { T_NORMAL, T_NONE, 160, 110, 65, 30, 65, 25, 189, G_SLOW, 23, 933, 7, 0x32A8AFD7AFFFB1ULL },  // 143 snorlax
  { T_ICE, T_FLYING, 90, 85, 100, 85, 125, 3, 261, G_SLOW, 25, 940, 5, 0x0A0C43C0087F2AULL },  // 144 articuno
  { T_ELECTRIC, T_FLYING, 90, 90, 85, 100, 125, 3, 261, G_SLOW, 25, 945, 5, 0x4A1C43C188432AULL },  // 145 zapdos
  { T_FIRE, T_FLYING, 90, 100, 90, 90, 125, 3, 261, G_SLOW, 25, 950, 5, 0x0A0C63C008432AULL },  // 146 moltres
  { T_DRAGON, T_NONE, 41, 64, 45, 50, 50, 45, 60, G_SLOW, 21, 955, 7, 0x1218E3C1C83FA0ULL },  // 147 dratini
  { T_DRAGON, T_NONE, 61, 84, 65, 70, 70, 45, 147, G_SLOW, 21, 962, 7, 0x1218E3C1C83FE0ULL },  // 148 dragonair
  { T_DRAGON, T_FLYING, 91, 134, 95, 80, 100, 45, 270, G_SLOW, 21, 969, 7, 0x3218E3C1C87FE2ULL },  // 149 dragonite
  { T_PSYCHIC, T_NONE, 106, 110, 90, 130, 154, 3, 306, G_SLOW, 25, 976, 8, 0x6338AFF1AFFFB1ULL },  // 150 mewtwo
  { T_PSYCHIC, T_NONE, 100, 100, 100, 100, 100, 45, 270, G_MEDIUM_SLOW, 21, 984, 5, 0x7FFFFFFFFFFFFFULL },  // 151 mew
};

// ---- evoluciones de gen 1 (nivel, piedra o intercambio) ----
struct EvoEntry { uint8_t from, to, method, param; };  // param: nivel o Stone
#define NUM_EVOS 72
static const EvoEntry EVO_TBL[NUM_EVOS] = {
  { 1, 2, EVO_LEVEL, 16 },
  { 2, 3, EVO_LEVEL, 32 },
  { 4, 5, EVO_LEVEL, 16 },
  { 5, 6, EVO_LEVEL, 36 },
  { 7, 8, EVO_LEVEL, 16 },
  { 8, 9, EVO_LEVEL, 36 },
  { 10, 11, EVO_LEVEL, 7 },
  { 11, 12, EVO_LEVEL, 10 },
  { 13, 14, EVO_LEVEL, 7 },
  { 14, 15, EVO_LEVEL, 10 },
  { 16, 17, EVO_LEVEL, 18 },
  { 17, 18, EVO_LEVEL, 36 },
  { 19, 20, EVO_LEVEL, 20 },
  { 21, 22, EVO_LEVEL, 20 },
  { 23, 24, EVO_LEVEL, 22 },
  { 25, 26, EVO_STONE, STONE_THUNDER },
  { 27, 28, EVO_LEVEL, 22 },
  { 29, 30, EVO_LEVEL, 16 },
  { 30, 31, EVO_STONE, STONE_MOON },
  { 32, 33, EVO_LEVEL, 16 },
  { 33, 34, EVO_STONE, STONE_MOON },
  { 35, 36, EVO_STONE, STONE_MOON },
  { 37, 38, EVO_STONE, STONE_FIRE },
  { 39, 40, EVO_STONE, STONE_MOON },
  { 41, 42, EVO_LEVEL, 22 },
  { 43, 44, EVO_LEVEL, 21 },
  { 44, 45, EVO_STONE, STONE_LEAF },
  { 46, 47, EVO_LEVEL, 24 },
  { 48, 49, EVO_LEVEL, 31 },
  { 50, 51, EVO_LEVEL, 26 },
  { 52, 53, EVO_LEVEL, 28 },
  { 54, 55, EVO_LEVEL, 33 },
  { 56, 57, EVO_LEVEL, 28 },
  { 58, 59, EVO_STONE, STONE_FIRE },
  { 60, 61, EVO_LEVEL, 25 },
  { 61, 62, EVO_STONE, STONE_WATER },
  { 63, 64, EVO_LEVEL, 16 },
  { 64, 65, EVO_TRADE, 0 },
  { 66, 67, EVO_LEVEL, 28 },
  { 67, 68, EVO_TRADE, 0 },
  { 69, 70, EVO_LEVEL, 21 },
  { 70, 71, EVO_STONE, STONE_LEAF },
  { 72, 73, EVO_LEVEL, 30 },
  { 74, 75, EVO_LEVEL, 25 },
  { 75, 76, EVO_TRADE, 0 },
  { 77, 78, EVO_LEVEL, 40 },
  { 79, 80, EVO_LEVEL, 37 },
  { 81, 82, EVO_LEVEL, 30 },
  { 84, 85, EVO_LEVEL, 31 },
  { 86, 87, EVO_LEVEL, 34 },
  { 88, 89, EVO_LEVEL, 38 },
  { 90, 91, EVO_STONE, STONE_WATER },
  { 92, 93, EVO_LEVEL, 25 },
  { 93, 94, EVO_TRADE, 0 },
  { 96, 97, EVO_LEVEL, 26 },
  { 98, 99, EVO_LEVEL, 28 },
  { 100, 101, EVO_LEVEL, 30 },
  { 102, 103, EVO_STONE, STONE_LEAF },
  { 104, 105, EVO_LEVEL, 28 },
  { 109, 110, EVO_LEVEL, 35 },
  { 111, 112, EVO_LEVEL, 42 },
  { 116, 117, EVO_LEVEL, 32 },
  { 118, 119, EVO_LEVEL, 33 },
  { 120, 121, EVO_STONE, STONE_WATER },
  { 129, 130, EVO_LEVEL, 20 },
  { 133, 134, EVO_STONE, STONE_WATER },
  { 133, 135, EVO_STONE, STONE_THUNDER },
  { 133, 136, EVO_STONE, STONE_FIRE },
  { 138, 139, EVO_LEVEL, 40 },
  { 140, 141, EVO_LEVEL, 40 },
  { 147, 148, EVO_LEVEL, 30 },
  { 148, 149, EVO_LEVEL, 55 },
};

static inline const char *typeName(uint8_t t) {
  if (t >= T_COUNT) return "";
  return gLang == LANG_ES ? TYPE_NAME_ES[t] : TYPE_NAME_EN[t];
}

static inline const char *moveName(uint8_t id) {
  if (id == 0 || id > NUM_MOVES) return MOVE_TBL[0].en;
  return gLang == LANG_ES ? MOVE_TBL[id].es : MOVE_TBL[id].en;
}

// multiplicador x2 contra un defensor de uno o dos tipos: 0, 1, 2, 4 u 8 (= x4)
static inline uint8_t typeEff2(uint8_t atk, uint8_t def1, uint8_t def2) {
  uint16_t m = TYPE_EFF[atk][def1];
  if (def2 != T_NONE) m = m * TYPE_EFF[atk][def2] / 2;
  return (uint8_t)m;
}
