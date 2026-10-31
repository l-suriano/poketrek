#include "rewards.h"
#include "gyms.h"

void progressInit(Progress &p) { p = Progress(); }

// el 1-1-1970 fue jueves: +3 hace que la semana empiece el lunes
uint16_t weekIndex(uint32_t now) { return (uint16_t)((dayIndex(now) + 3) / 7); }

void progressWeek(Progress &p, uint32_t now) {
  uint16_t w = weekIndex(now);
  if (w == p.week) return;
  p.week = w;
  p.featuredWins = 0;
  p.weeklyPrizeGiven = false;
  p.candiesBought = 0;
}

uint16_t wildShinyDen(const Progress &p) { return p.dcDays >= 30 ? 128 : 256; }

uint32_t cleanWinBonus(uint32_t amount, bool noneFainted, bool partnerInTeam) {
  if (noneFainted) amount = amount * 5 / 4;
  if (partnerInTeam) amount = amount * 5 / 4;
  return amount;
}

bool partnerInParty(const Trainer &t) {
  for (int i = 0; i < t.partyCount; i++) if (t.party[i].flags & OM_PARTNER) return true;
  return false;
}

// --------------------------------------------------------------- gimnasios -

// MT por tramo del as (<25, 25-44, 45+) y objeto extra; reglas: tabla de gimnasios
static const GymPrize GYM_PRIZES[NUM_GYMS][3] = {
  { {{34, 0}, 0, 0}, {{48, 0}, 0, 0}, {{26, 0}, 0, 0} },                                // Brock
  { {{12, 0}, 0, 0}, {{11, 0}, 0, 0}, {{13, 0}, 0, 0} },                                // Misty
  { {{45, 0}, 0, 0}, {{24, 0}, 0, 0}, {{25, 0}, 0, 0} },                                // Lt. Surge
  { {{21, 0}, 0, 0}, {{22, 0}, 0, 0}, {{22, 0}, IT_LEAF_STONE, 1} },                    // Erika
  { {{6, 0}, 0, 0}, {{6, 0}, IT_SUPER_POTION, 3}, {{6, 0}, IT_MOON_STONE, 1} },         // Koga
  { {{46, 0}, 0, 0}, {{29, 0}, 0, 0}, {{29, 42}, 0, 0} },                               // Sabrina
  { {{0, 0}, IT_FIRE_STONE, 1}, {{38, 0}, 0, 0}, {{38, 0}, IT_FIRE_STONE, 1} },         // Blaine
  { {{28, 0}, 0, 0}, {{26, 0}, 0, 0}, {{26, 27}, 0, 0} },                               // Giovanni
};

GymPrize gymPrize(uint8_t gym, uint8_t aceLevel) {
  if (gym >= NUM_GYMS) return GymPrize{{0, 0}, 0, 0};
  return GYM_PRIZES[gym][gymTmTier(aceLevel)];
}

GymResult gymVictory(Trainer &t, uint8_t gym, uint8_t aceLevel, uint8_t lastLevel,
                     bool noneFainted) {
  GymResult r;
  if (gym >= NUM_GYMS) return r;
  uint32_t money = prizeMoney(99, lastLevel);  // los lideres usan base 99
  if (!(t.badges & (1 << gym))) {
    t.badges |= (uint8_t)(1 << gym);
    r.newBadge = true;
    r.prize = gymPrize(gym, aceLevel);
    for (int i = 0; i < 2; i++)
      if (r.prize.tm[i]) {
        uint8_t &n = t.tms[r.prize.tm[i] - 1];
        if (n < 99) n++;
      }
    if (r.prize.item) addItem(t, r.prize.item, r.prize.itemCount);
    r.exp.firstLeaderWin = true;
  } else if (t.champion) {
    money /= 2;  // revancha: mitad de dinero y XP, sin MT
    r.exp.rematch = true;
  } else {
    money = 0;   // las recompensas son una vez por gimnasio
  }
  r.money = cleanWinBonus(money, noneFainted, partnerInParty(t));
  earn(t, r.money);
  return r;
}

void eliteVictory(Trainer &t, Progress &p, uint8_t idx) {
  if (idx >= NUM_ELITE) return;
  p.eliteBeaten |= (uint8_t)(1 << idx);
  if (idx != ELITE_CHAMPION) return;
  t.champion = true;
  if (!p.masterBallGiven) {
    addItem(t, IT_MASTER_BALL);
    p.masterBallGiven = true;
  }
}

// ---------------------------------------------------------- Daily Challenge -

bool dailyPlayedToday(const Progress &p, uint32_t now) { return p.dcLastDay == dayIndex(now); }

static bool giveMew(Progress &p, Trainer &t, Rng &rng) {
  OwnedMon m;
  ownedInit(m, 151, partyBaseLevel(t), rng);  // confianza 100, como un regalo
  if (!addOwned(t, m)) { p.mewPending = true; return false; }
  dexMark(t.seen, 151);
  dexMark(t.caught, 151);
  p.mewGiven = true;
  p.mewPending = false;
  return true;
}

void deliverPending(Progress &p, Trainer &t, Explore &ex, Rng &rng) {
  while (p.pendingEggs[0] && eggAdd(ex, rng, EGG_RARE)) p.pendingEggs[0]--;
  while (p.pendingEggs[1] && eggAdd(ex, rng, EGG_RARE, 1)) p.pendingEggs[1]--;
  if (p.mewPending) giveMew(p, t, rng);
}

bool dailyCountDay(Progress &p, Trainer &t, Explore &ex, Rng &rng, uint32_t now,
                   uint16_t *bpBonus) {
  uint16_t day = dayIndex(now);
  if (p.dcLastDay == day) return false;
  p.dcStreak = (p.dcLastDay != 0xFFFF && day == p.dcLastDay + 1) ? p.dcStreak + 1 : 1;
  p.dcLastDay = day;
  p.dcDays++;
  p.dcScore = 0;
  uint16_t bp = 0;
  if (p.dcStreak % 7 == 0) bp = 5;  // cada 7 dias seguidos
  t.bp += bp;
  if (bpBonus) *bpBonus = bp;

  switch (p.dcDays) {
    case 7: p.pendingEggs[0]++; break;               // Huevo Raro
    case 14: p.skins |= SKIN_GREAT; break;
    case 30: if (!p.mewGiven) p.mewPending = true; break;
    case 50: p.skins |= SKIN_ULTRA; p.backgrounds |= BG_EXCLUSIVE; break;
    case 100:                                        // Huevo shiny + skin + titulo
      p.pendingEggs[1]++;
      p.skins |= SKIN_MASTER;
      p.titles |= TITLE_KANTO_LEGEND;
      break;
  }
  deliverPending(p, t, ex, rng);
  return true;
}

// ----------------------------------------------------------------- guardado -

#define PROGRESS_VERSION 1

size_t progressSaveSize() { return SAVE_HEADER + sizeof(Progress); }

size_t progressSave(const Progress &p, uint8_t *buf, size_t cap) {
  return saveBlob("PTP1", PROGRESS_VERSION, &p, sizeof(Progress), buf, cap);
}

bool progressLoad(Progress &p, const uint8_t *buf, size_t len) {
  return loadBlob("PTP1", PROGRESS_VERSION, &p, sizeof(Progress), buf, len);
}
