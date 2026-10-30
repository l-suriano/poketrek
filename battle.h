#pragma once
#include <stdint.h>
#include "battle_data.h"

// Motor de batallas de PokeTrek: reglas de gen 1 (Rojo/Azul) con los ajustes
// de las reglas del juego. C++ puro sin Arduino, asi que el mismo codigo corre
// en la placa, en los tests y en el simulador del PC (sim/).
//
// Cambios respecto a gen 1, a proposito:
// - Fantasma > Psiquico es x2 (en battle_data.h) y Foco Energia sube el critico.
// - Los ataques con 100 % de precision nunca fallan (sin el fallo de 1/256).
// - La congelacion se cura con un 20 % por turno (en gen 1 era para siempre).
// - Atadura, Constriccion, Giro Fuego y Tenaza quitan 1/8 por turno durante
//   2-5 turnos pero no impiden atacar (en gen 1 bloqueaban al rival entero).

#define PARTY_MAX 6
#define BATTLE_EVENTS 48

// RNG xorshift32 propio: reproducible (Daily Challenge, tests) y sin depender
// del random() de Arduino.
struct Rng {
  uint32_t s = 0x2545F491u;
  void seed(uint32_t v) { s = v ? v : 0x2545F491u; }
  uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
  int range(int lo, int hi) { return lo + (int)(next() % (uint32_t)(hi - lo + 1)); }  // [lo, hi]
  bool chance(int pct) { return (int)(next() % 100) < pct; }
};

enum Status : uint8_t { STS_NONE = 0, STS_PAR, STS_SLP, STS_FRZ, STS_BRN, STS_PSN, STS_TOX };

// indices de las etapas de stat (-6..+6)
enum Stage : uint8_t { SG_ATK = 0, SG_DEF, SG_SPE, SG_SPC, SG_ACC, SG_EVA, SG_COUNT };

struct BattleMon {
  uint8_t species = 0;  // 0 = hueco vacio
  uint8_t level = 1;
  uint8_t type1 = T_NORMAL, type2 = T_NONE;
  uint16_t maxHp = 1, hp = 1;
  uint16_t atk = 1, def = 1, spe = 1, spc = 1;  // stats ya calculadas
  uint8_t moves[4] = {0, 0, 0, 0};
  uint8_t pp[4] = {0, 0, 0, 0};
  uint8_t status = STS_NONE;
  uint8_t sleepTurns = 0;   // turnos de sueno que quedan
  uint8_t toxicN = 0;       // contador de Toxico (n/16)
  uint8_t trust = 100;      // 0..100; probabilidad de desobedecer = (100 - trust) / 2
};

// estado volatil del Pokemon en el campo (se borra al cambiar)
struct Volatile {
  int8_t stage[SG_COUNT] = {0, 0, 0, 0, 0, 0};
  uint8_t confused = 0;      // turnos de confusion que quedan
  uint8_t trapTurns = 0;     // turnos de Atadura y similares (1/8 por turno)
  uint8_t charging = 0;      // movimiento de dos turnos en carga
  bool invulnerable = false; // Vuelo / Excavar en carga
  bool recharge = false;     // tras Hiperrayo
  uint8_t lockMove = 0;      // Golpe / Danza Petalo: movimiento obligado
  uint8_t lockTurns = 0;
  uint8_t bideTurns = 0;
  uint16_t bideDamage = 0;
  bool seeded = false;       // Drenadoras
  bool focus = false;        // Foco Energia (x4 al critico)
  bool flinch = false;
  bool reflect = false, lightScreen = false, mist = false;
  uint16_t substituteHp = 0;
  uint8_t disabledMove = 0, disabledTurns = 0;
  uint8_t lastMove = 0;      // para Mov. Espejo
  uint16_t lastDamage = 0;   // dano recibido este turno (Contraataque)
  uint8_t lastDamageType = T_NORMAL;
  bool transformed = false;
};

struct BattleSide {
  BattleMon party[PARTY_MAX];
  uint8_t count = 0;
  uint8_t active = 0;
  Volatile vol;
  uint8_t aiTier = 1;      // 1..4 (ver battleAiChoose)
  uint8_t potions = 0;     // curas que puede usar en esta batalla
  uint16_t potionHeal = 20;
  // Transformacion / Conversion / Mimetismo cambian al Pokemon solo mientras
  // esta en el campo: aqui se guarda como era para devolverlo al salir
  BattleMon saved;
  bool hasSaved = false;
};

enum ActionKind : uint8_t { ACT_MOVE, ACT_SWITCH, ACT_ITEM, ACT_RUN };
struct Action { uint8_t kind = ACT_MOVE; uint8_t index = 0; };  // index: hueco de movimiento o de equipo

enum Outcome : uint8_t { OUT_NONE = 0, OUT_WIN0, OUT_WIN1, OUT_FLED };

// Eventos para la UI (animaciones y textos). El motor solo los apunta.
enum EvKind : uint8_t {
  EV_MOVE, EV_MISS, EV_DAMAGE, EV_CRIT, EV_EFFECT, EV_STATUS, EV_STAT, EV_HEAL,
  EV_FAINT, EV_SWITCH, EV_FAIL, EV_FLINCH, EV_CONFUSED_HIT, EV_FULL_PARA,
  EV_ASLEEP, EV_FROZEN, EV_THAW, EV_WAKE, EV_RECHARGE, EV_CHARGE, EV_DISOBEY, EV_ITEM,
};
struct BattleEvent { uint8_t kind, side, a; int16_t b; };

struct Battle {
  BattleSide side[2];
  Rng rng;
  bool wild = false;
  uint8_t outcome = OUT_NONE;
  uint16_t turn = 0;
  BattleEvent ev[BATTLE_EVENTS];
  uint8_t evCount = 0;  // eventos del ultimo turno
};

// ---- crear Pokemon ----
// genes 90..110 (%), statExp 0..65535 por stat (HP, Atk, Def, Spe, Spc)
void monInit(BattleMon &m, uint8_t species, uint8_t level, const uint8_t genes[5] = nullptr,
             const uint16_t statExp[5] = nullptr);
// los 4 ultimos movimientos aprendidos hasta `level` (como un Pokemon salvaje de gen 1)
void monDefaultMoves(BattleMon &m);
void monSetMove(BattleMon &m, uint8_t slot, uint8_t move);
// evoluciona `species` a la forma que tendria un entrenador a `level`
// (piedras desde el 30, intercambio desde el 35); `prefer` decide la rama de Eevee
uint8_t evolveForLevel(uint8_t species, uint8_t level, uint8_t prefer = T_NONE);

// ---- batalla ----
void battleInit(Battle &b, uint32_t seed, bool wild);
BattleMon &activeMon(Battle &b, int s);
const BattleMon &activeMon(const Battle &b, int s);
// resuelve un turno completo con las acciones de los dos lados
void battleTurn(Battle &b, Action a0, Action a1);
// tras un debilitamiento: el lado `s` debe meter a otro (true si le queda alguno)
bool battleNeedsSwitch(const Battle &b, int s);
void battleSwitchIn(Battle &b, int s, uint8_t idx);
bool canUseMove(const Battle &b, int s, uint8_t slot);

// ---- IA ----
// Nivel 1 al azar, 2 prefiere supereficaces, 3 ademas cambia, cura y usa estados,
// 4 ademas prevé cambios y guarda el golpe mas fuerte para rematar.
uint8_t aiTierForLevel(uint8_t level);
Action battleAiChoose(Battle &b, int s);
uint8_t battleAiPickSwitch(Battle &b, int s);  // a quien meter tras un debilitamiento

// ---- calculos sueltos (tambien los usa la UI) ----
// dano medio aproximado (sin critico ni azar), para la IA y la UI
uint16_t estimateDamage(const BattleMon &att, const BattleMon &def, uint8_t move);
// probabilidad de captura en tantos por mil (reglas: Catching)
// ball: 10 = Poke Ball, 15 = Super Ball, 20 = Ultra Ball, 0 = Master Ball; throwX10: 10/13/16/20
uint16_t catchChancePermille(const BattleMon &wild, uint8_t ball, uint8_t throwX10);
