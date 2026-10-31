#include "trainer.h"
#include <string.h>

// ---------------------------------------------------------------- objetos --

// nombres ES con acentos en CP437 octal, como battle_data.h e i18n.cpp
const ItemInfo ITEMS[IT_COUNT] = {
  { "Pok\202 Ball", "Pok\202 Ball", 200, PK_BALLS },
  { "Great Ball", "Super Ball", 600, PK_BALLS },
  { "Ultra Ball", "Ultra Ball", 1200, PK_BALLS },
  { "Master Ball", "Master Ball", 0, PK_BALLS },
  { "Potion", "Poci\242n", 300, PK_MEDICINE },
  { "Super Potion", "Superpoci\242n", 700, PK_MEDICINE },
  { "Hyper Potion", "Hiperpoci\242n", 1500, PK_MEDICINE },
  { "Max Potion", "Poci\242n M\240xima", 2500, PK_MEDICINE },
  { "Full Restore", "Restaurar Todo", 3000, PK_MEDICINE },
  { "Revive", "Revivir", 1500, PK_MEDICINE },
  { "Max Revive", "Revivir M\240ximo", 0, PK_MEDICINE },
  { "Antidote", "Ant\241doto", 100, PK_MEDICINE },
  { "Paralyze Heal", "Antiparalizador", 200, PK_MEDICINE },
  { "Awakening", "Despertar", 250, PK_MEDICINE },
  { "Burn Heal", "Antiquemar", 250, PK_MEDICINE },
  { "Ice Heal", "Antihielo", 250, PK_MEDICINE },
  { "Full Heal", "Cura Total", 600, PK_MEDICINE },
  { "Fire Stone", "Piedra Fuego", 2100, PK_ITEMS },
  { "Water Stone", "Piedra Agua", 2100, PK_ITEMS },
  { "Thunder Stone", "Piedra Trueno", 2100, PK_ITEMS },
  { "Leaf Stone", "Piedra Hoja", 2100, PK_ITEMS },
  { "Moon Stone", "Piedra Lunar", 0, PK_ITEMS },
  { "HP Up", "M\240s PS", 9800, PK_ITEMS },
  { "Protein", "Prote\241na", 9800, PK_ITEMS },
  { "Iron", "Hierro", 9800, PK_ITEMS },
  { "Carbos", "Carburante", 9800, PK_ITEMS },
  { "Calcium", "Calcio", 9800, PK_ITEMS },
  { "Rare Candy", "Caramelo Raro", 0, PK_ITEMS },
  { "Nugget", "Pepita", 10000, PK_ITEMS },   // solo se vende: 5000 (la mitad)
  { "Incense", "Incienso", 0, PK_ITEMS },
  { "Link Cable", "Cable Link", 0, PK_KEY },
  { "Exp. Share", "Repartir Exp.", 0, PK_KEY },
};

const char *itemName(uint8_t item) {
  if (item >= IT_COUNT) return "";
  return gLang == LANG_ES ? ITEMS[item].es : ITEMS[item].en;
}

void addItem(Trainer &t, uint8_t item, uint16_t n) {
  if (item >= IT_COUNT) return;
  uint32_t v = (uint32_t)t.bag[item] + n;
  t.bag[item] = (uint16_t)(v > 99 ? 99 : v);  // bolsillos de 99, como en las reglas
}

// ---------------------------------------------------------------- niveles --

uint32_t expForLevel(uint8_t growth, uint8_t level) {
  if (level <= 1) return 0;
  int64_t n = level;
  int64_t v;
  switch (growth) {
    case G_FAST: v = 4 * n * n * n / 5; break;
    case G_MEDIUM_SLOW: v = 6 * n * n * n / 5 - 15 * n * n + 100 * n - 140; break;
    case G_SLOW: v = 5 * n * n * n / 4; break;
    default: v = n * n * n; break;  // G_MEDIUM
  }
  return v < 0 ? 0 : (uint32_t)v;
}

uint32_t battleExp(uint8_t defeatedSpecies, uint8_t defeatedLevel, ExpBonus bonus) {
  uint64_t xp = (uint64_t)BSPECIES[defeatedSpecies].baseExp * defeatedLevel / 7;
  if (bonus.trainer) xp = xp * 3 / 2;
  if (bonus.firstLeaderWin) xp *= 2;
  if (bonus.featured) xp = xp * 3 / 2;
  if (bonus.rematch) xp /= 2;
  return xp ? (uint32_t)xp : 1;
}

uint16_t ownedMaxHp(const OwnedMon &m) {
  BattleMon b;
  monInit(b, m.species, m.level, m.genes, m.statExp);
  return b.maxHp;
}

static bool knowsMove(const OwnedMon &m, uint8_t move) {
  for (int i = 0; i < 4; i++) if (m.moves[i] == move) return true;
  return false;
}

LevelUp gainExp(OwnedMon &m, uint32_t exp) {
  LevelUp r;
  if (!m.species || m.level >= 100) return r;
  uint8_t growth = BSPECIES[m.species].growth;
  uint32_t cap = expForLevel(growth, 100);
  m.exp = (cap - m.exp < exp) ? cap : m.exp + exp;
  uint16_t oldMax = ownedMaxHp(m);
  const BattleSpecies &s = BSPECIES[m.species];
  while (m.level < 100 && m.exp >= expForLevel(growth, m.level + 1)) {
    m.level++;
    r.levels++;
    for (int i = 0; i < s.learnCount; i++) {
      const LearnEntry &e = LEARN_TBL[s.learnStart + i];
      if (e.level == m.level && !knowsMove(m, e.move) && r.newMoveCount < MAX_NEW_MOVES)
        r.newMoves[r.newMoveCount++] = e.move;
    }
  }
  if (r.levels && m.hp) m.hp += ownedMaxHp(m) - oldMax;  // gen 1: subir de nivel suma lo que sube el maximo
  r.evolveTo = levelEvolution(m);
  return r;
}

void addStatExp(OwnedMon &m, uint8_t defeatedSpecies) {
  const BattleSpecies &d = BSPECIES[defeatedSpecies];
  const uint8_t base[5] = { d.hp, d.atk, d.def, d.spe, d.spc };
  for (int i = 0; i < 5; i++) {
    uint32_t v = (uint32_t)m.statExp[i] + base[i];
    m.statExp[i] = (uint16_t)(v > 65535 ? 65535 : v);
  }
}

bool learnMove(OwnedMon &m, uint8_t move, uint8_t replaceSlot) {
  if (!move || move > NUM_MOVES || knowsMove(m, move)) return false;
  int slot = -1;
  for (int i = 0; i < 4 && slot < 0; i++) if (!m.moves[i]) slot = i;
  if (slot < 0) {
    if (replaceSlot >= 4) return false;
    slot = replaceSlot;
  }
  m.moves[slot] = move;
  m.pp[slot] = MOVE_TBL[move].pp;
  return true;
}

bool canLearnMachine(uint8_t species, uint8_t machineIdx) {
  return machineIdx < NUM_MACHINES && ((BSPECIES[species].machines >> machineIdx) & 1ULL);
}

// ---------------------------------------------------------------- crear ----

void ownedInit(OwnedMon &m, uint8_t species, uint8_t level, Rng &rng) {
  m = OwnedMon();
  m.species = species;
  m.level = level;
  m.exp = expForLevel(BSPECIES[species].growth, level);
  for (int i = 0; i < 5; i++) m.genes[i] = (uint8_t)rng.range(90, 110);
  BattleMon b;
  monInit(b, species, level, m.genes, m.statExp);
  for (int i = 0; i < 4; i++) { m.moves[i] = b.moves[i]; m.pp[i] = b.pp[i]; }
  m.hp = b.maxHp;
}

void toBattle(const OwnedMon &m, BattleMon &out) {
  monInit(out, m.species, m.level, m.genes, m.statExp);
  for (int i = 0; i < 4; i++) { out.moves[i] = m.moves[i]; out.pp[i] = m.pp[i]; }
  out.hp = m.hp > out.maxHp ? out.maxHp : m.hp;
  out.status = m.status;
  out.sleepTurns = m.status == STS_SLP ? 2 : 0;
  out.trust = m.trust;
}

void fromBattle(OwnedMon &m, const BattleMon &b) {
  m.hp = b.hp;
  for (int i = 0; i < 4; i++) m.pp[i] = b.pp[i];
  m.status = b.status == STS_TOX ? (uint8_t)STS_PSN : b.status;  // fuera de combate Toxico es veneno normal
}

// ---------------------------------------------------------------- evolucion

uint8_t levelEvolution(const OwnedMon &m) {
  for (int i = 0; i < NUM_EVOS; i++) {
    const EvoEntry &e = EVO_TBL[i];
    if (e.from == m.species && e.method == EVO_LEVEL && m.level >= e.param) return e.to;
  }
  return 0;
}

uint8_t stoneEvolution(uint8_t species, uint8_t stoneItem) {
  int stone = stoneItem == IT_FIRE_STONE ? STONE_FIRE : stoneItem == IT_WATER_STONE ? STONE_WATER :
              stoneItem == IT_THUNDER_STONE ? STONE_THUNDER : stoneItem == IT_LEAF_STONE ? STONE_LEAF :
              stoneItem == IT_MOON_STONE ? STONE_MOON : -1;
  if (stone < 0) return 0;
  for (int i = 0; i < NUM_EVOS; i++) {
    const EvoEntry &e = EVO_TBL[i];
    if (e.from == species && e.method == EVO_STONE && e.param == stone) return e.to;
  }
  return 0;
}

uint8_t tradeEvolution(uint8_t species) {
  for (int i = 0; i < NUM_EVOS; i++)
    if (EVO_TBL[i].from == species && EVO_TBL[i].method == EVO_TRADE) return EVO_TBL[i].to;
  return 0;
}

void evolve(OwnedMon &m, uint8_t newSpecies) {
  if (!newSpecies) return;
  uint16_t oldMax = ownedMaxHp(m);
  m.species = newSpecies;
  if (m.hp) m.hp += ownedMaxHp(m) - oldMax;
}

// ---------------------------------------------------------------- confianza

uint8_t startingTrust(uint8_t caughtLevel, uint8_t highestLevel, bool traded) {
  if (traded) return 40;
  if (caughtLevel > highestLevel + 5) return 30;
  return 100;
}

void trustAfterBattle(OwnedMon &m) {
  uint8_t add = m.trustToday >= 10 ? 0 : (uint8_t)(10 - m.trustToday < 2 ? 10 - m.trustToday : 2);
  m.trustToday += add;
  m.trust = (uint8_t)(m.trust + add > 100 ? 100 : m.trust + add);
}

void trustPerKm(OwnedMon &m, uint16_t km) {
  uint32_t v = (uint32_t)m.trust + km;
  m.trust = (uint8_t)(v > 100 ? 100 : v);
}

void trustTournamentWin(OwnedMon &m) { m.trust = (uint8_t)(m.trust + 5 > 100 ? 100 : m.trust + 5); }

void trustNewDay(Trainer &t) {
  for (int i = 0; i < t.partyCount; i++) t.party[i].trustToday = 0;
  for (int i = 0; i < t.boxCount; i++) t.box[i].trustToday = 0;
}

// ---------------------------------------------------------------- captura --

uint8_t ballMultX10(uint8_t ballItem) {
  switch (ballItem) {
    case IT_GREAT_BALL: return 15;
    case IT_ULTRA_BALL: return 20;
    case IT_MASTER_BALL: return 0;
    default: return 10;
  }
}

bool catchAttempt(Rng &rng, const BattleMon &wild, uint8_t ballItem, uint8_t throwX10, uint8_t *shakes) {
  uint16_t pm = catchChancePermille(wild, ballMultX10(ballItem), throwX10);
  bool ok = (rng.next() % 1000) < pm;
  if (shakes) *shakes = ok ? 3 : (uint8_t)rng.range(0, 2);
  return ok;
}

bool addOwned(Trainer &t, const OwnedMon &m) {
  if (t.partyCount < PARTY_MAX) { t.party[t.partyCount++] = m; return true; }
  if (t.boxCount < BOX_MAX) { t.box[t.boxCount++] = m; return true; }
  return false;
}

bool addCaught(Trainer &t, const BattleMon &wild, Rng &rng, bool shiny) {
  OwnedMon m;
  ownedInit(m, wild.species, wild.level, rng);
  for (int i = 0; i < 4; i++) { m.moves[i] = wild.moves[i]; m.pp[i] = wild.pp[i]; }
  // los genes se tiran de nuevo: conserva la proporcion de PS que tenia
  uint16_t maxHp = ownedMaxHp(m);
  uint32_t hp = wild.maxHp ? (uint32_t)wild.hp * maxHp / wild.maxHp : maxHp;
  m.hp = (uint16_t)(hp ? hp : 1);
  m.status = wild.status == STS_TOX ? (uint8_t)STS_PSN : wild.status;
  m.trust = startingTrust(wild.level, highestPartyLevel(t), false);
  if (shiny) m.flags |= OM_SHINY;
  if (!addOwned(t, m)) return false;
  dexMark(t.seen, wild.species);
  dexMark(t.caught, wild.species);
  return true;
}

// ---------------------------------------------------------------- equipo ---

uint8_t highestPartyLevel(const Trainer &t) {
  uint8_t hi = 1;
  for (int i = 0; i < t.partyCount; i++) if (t.party[i].level > hi) hi = t.party[i].level;
  return hi;
}

bool anyAlive(const Trainer &t) {
  for (int i = 0; i < t.partyCount; i++) if (t.party[i].hp) return true;
  return false;
}

bool depositToBox(Trainer &t, uint8_t idx) {
  if (idx >= t.partyCount || t.partyCount <= 1 || t.boxCount >= BOX_MAX) return false;
  if (t.party[idx].flags & OM_PARTNER) return false;  // el compañero siempre va en el equipo
  t.box[t.boxCount++] = t.party[idx];
  for (int i = idx; i < t.partyCount - 1; i++) t.party[i] = t.party[i + 1];
  t.party[--t.partyCount] = OwnedMon();
  return true;
}

bool withdrawFromBox(Trainer &t, uint8_t idx) {
  if (idx >= t.boxCount || t.partyCount >= PARTY_MAX) return false;
  t.party[t.partyCount++] = t.box[idx];
  for (int i = idx; i < t.boxCount - 1; i++) t.box[i] = t.box[i + 1];
  t.box[--t.boxCount] = OwnedMon();
  return true;
}

void healAll(Trainer &t) {
  for (int i = 0; i < t.partyCount; i++) {
    OwnedMon &m = t.party[i];
    m.hp = ownedMaxHp(m);
    m.status = STS_NONE;
    for (int k = 0; k < 4; k++) m.pp[k] = m.moves[k] ? MOVE_TBL[m.moves[k]].pp : 0;
  }
}

// ---------------------------------------------------------------- objetos --

static UseResult healBy(OwnedMon &m, uint32_t amount) {
  uint16_t maxHp = ownedMaxHp(m);
  if (!m.hp || m.hp >= maxHp) return USE_NO_EFFECT;
  uint32_t v = m.hp + amount * HP_MULT;  // las curas fijas escalan con los PS
  m.hp = (uint16_t)(v > maxHp ? maxHp : v);
  return USE_OK;
}

static UseResult cure(OwnedMon &m, uint8_t a, uint8_t b = 0xFF) {
  if (!m.hp || !m.status || (a != 0xFF && m.status != a && m.status != b)) return USE_NO_EFFECT;
  m.status = STS_NONE;
  return USE_OK;
}

UseResult useItemOn(Trainer &t, uint8_t item, OwnedMon &m, LevelUp *lv) {
  if (item >= IT_COUNT || !t.bag[item]) return USE_NO_ITEM;
  uint16_t maxHp = ownedMaxHp(m);
  UseResult r = USE_NO_EFFECT;
  switch (item) {
    case IT_POTION: r = healBy(m, 20); break;
    case IT_SUPER_POTION: r = healBy(m, 50); break;
    case IT_HYPER_POTION: r = healBy(m, 200); break;
    case IT_MAX_POTION: r = healBy(m, maxHp); break;
    case IT_FULL_RESTORE:
      if (m.hp && (m.hp < maxHp || m.status)) { m.hp = maxHp; m.status = STS_NONE; r = USE_OK; }
      break;
    case IT_REVIVE: if (!m.hp) { m.hp = maxHp / 2 ? maxHp / 2 : 1; m.status = STS_NONE; r = USE_OK; } break;
    case IT_MAX_REVIVE: if (!m.hp) { m.hp = maxHp; m.status = STS_NONE; r = USE_OK; } break;
    case IT_ANTIDOTE: r = cure(m, STS_PSN, STS_TOX); break;
    case IT_PARALYZE_HEAL: r = cure(m, STS_PAR); break;
    case IT_AWAKENING: r = cure(m, STS_SLP); break;
    case IT_BURN_HEAL: r = cure(m, STS_BRN); break;
    case IT_ICE_HEAL: r = cure(m, STS_FRZ); break;
    case IT_FULL_HEAL: r = cure(m, 0xFF); break;
    case IT_FIRE_STONE: case IT_WATER_STONE: case IT_THUNDER_STONE: case IT_LEAF_STONE: case IT_MOON_STONE: {
      uint8_t to = stoneEvolution(m.species, item);
      if (to) { evolve(m, to); dexMark(t.seen, to); dexMark(t.caught, to); r = USE_EVOLVED; }
      break;
    }
    case IT_HP_UP: case IT_PROTEIN: case IT_IRON: case IT_CARBOS: case IT_CALCIUM: {
      int k = item - IT_HP_UP;
      if (m.statExp[k] < 25600) {  // gen 1: las vitaminas solo llegan hasta 25600
        uint32_t v = m.statExp[k] + 2560u;
        m.statExp[k] = (uint16_t)(v > 25600 ? 25600 : v);
        if (m.hp) m.hp += ownedMaxHp(m) - maxHp;
        r = USE_OK;
      }
      break;
    }
    case IT_RARE_CANDY:
      if (m.level < 100) {
        uint32_t need = expForLevel(BSPECIES[m.species].growth, m.level + 1) - m.exp;
        LevelUp up = gainExp(m, need);
        if (lv) *lv = up;
        r = USE_LEVEL_UP;
      }
      break;
    default: break;
  }
  if (r != USE_NO_EFFECT) t.bag[item]--;
  return r;
}

// ---------------------------------------------------------------- dinero ---

bool spend(Trainer &t, uint32_t amount) {
  if (t.money < amount) return false;
  t.money -= amount;
  return true;
}

void earn(Trainer &t, uint32_t amount) {
  uint64_t v = (uint64_t)t.money + amount;
  t.money = (uint32_t)(v > 999999 ? 999999 : v);
}

uint32_t prizeMoney(uint8_t classBase, uint8_t lastLevel) { return (uint32_t)classBase * lastLevel; }

void blackout(Trainer &t) { t.money -= t.money / 10; }

uint8_t badgeCount(const Trainer &t) {
  uint8_t n = 0;
  for (int i = 0; i < NUM_BADGES; i++) n += (t.badges >> i) & 1;
  return n;
}

// medallas necesarias para que el Poke Mart venda cada objeto (255 = no lo vende)
static const uint8_t MART_BADGES[IT_COUNT] = {
  0, 2, 5, 255,          // bolas
  0, 2, 5, 7, 7,         // pociones y Restaurar Todo
  3, 255,                // Revivir
  0, 0, 0, 1, 1, 5,      // curas de estado
  255, 255, 255, 255, 255,  // piedras: Celadon Dept. Store
  255, 255, 255, 255, 255,  // vitaminas: Celadon Dept. Store
  255, 255, 255, 255, 255,
};

bool martSells(const Trainer &t, uint8_t item) {
  return item < IT_COUNT && MART_BADGES[item] != 255 && badgeCount(t) >= MART_BADGES[item];
}

void dexMark(uint8_t *bits, uint8_t species) {
  if (species >= 1 && species <= 151) bits[(species - 1) / 8] |= (uint8_t)(1 << ((species - 1) % 8));
}

bool dexHas(const uint8_t *bits, uint8_t species) {
  return species >= 1 && species <= 151 && (bits[(species - 1) / 8] >> ((species - 1) % 8)) & 1;
}

uint8_t dexCount(const uint8_t *bits) {
  uint8_t n = 0;
  for (int s = 1; s <= 151; s++) n += dexHas(bits, (uint8_t)s);
  return n;
}

// ---------------------------------------------------------------- guardado -

#define SAVE_VERSION 1

static uint32_t crc32(const uint8_t *p, size_t n) {
  uint32_t c = 0xFFFFFFFFu;
  for (size_t i = 0; i < n; i++) {
    c ^= p[i];
    for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
  }
  return ~c;
}

static void put32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static uint32_t get32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

size_t saveBlob(const char magic[4], uint16_t version, const void *data, size_t size,
                uint8_t *buf, size_t cap) {
  if (cap < SAVE_HEADER + size) return 0;
  memcpy(buf, magic, 4);
  buf[4] = version & 0xFF; buf[5] = version >> 8;
  buf[6] = buf[7] = 0;
  put32(buf + 8, (uint32_t)size);
  memcpy(buf + SAVE_HEADER, data, size);
  put32(buf + 12, crc32(buf + SAVE_HEADER, size));
  return SAVE_HEADER + size;
}

bool loadBlob(const char magic[4], uint16_t version, void *data, size_t size,
              const uint8_t *buf, size_t len) {
  if (len < SAVE_HEADER || memcmp(buf, magic, 4) != 0) return false;
  if ((buf[4] | buf[5] << 8) != version) return false;
  if (get32(buf + 8) != size || len < SAVE_HEADER + size) return false;
  if (crc32(buf + SAVE_HEADER, size) != get32(buf + 12)) return false;
  memcpy(data, buf + SAVE_HEADER, size);
  return true;
}

size_t trainerSaveSize() { return SAVE_HEADER + sizeof(Trainer); }

size_t trainerSave(const Trainer &t, uint8_t *buf, size_t cap) {
  return saveBlob("PTK1", SAVE_VERSION, &t, sizeof(Trainer), buf, cap);
}

bool trainerLoad(Trainer &t, const uint8_t *buf, size_t len) {
  return loadBlob("PTK1", SAVE_VERSION, &t, sizeof(Trainer), buf, len);
}
