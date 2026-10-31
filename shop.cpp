#include "shop.h"

static bool bagRoom(const Trainer &t, uint8_t item, uint8_t qty) { return t.bag[item] + qty <= 99; }

static ShopResult buyFor(Trainer &t, uint8_t item, uint8_t qty, uint32_t unit) {
  if (!qty) return SHOP_LIMIT;
  if (!bagRoom(t, item, qty)) return SHOP_BAG_FULL;
  if (!spend(t, unit * qty)) return SHOP_NO_MONEY;
  addItem(t, item, qty);
  return SHOP_OK;
}

void giveTm(Trainer &t, uint8_t tmNumber) {
  if (tmNumber < 1 || tmNumber > NUM_TMS) return;
  uint8_t &n = t.tms[tmNumber - 1];
  if (n < 99) n++;
}

// ---------------------------------------------------------------- Poke Mart -

ShopResult martBuy(Trainer &t, uint8_t item, uint8_t qty) {
  if (item >= IT_COUNT || !martSells(t, item)) return SHOP_NOT_SOLD;
  return buyFor(t, item, qty, ITEMS[item].price);
}

uint32_t sellPrice(uint8_t item) {
  if (item >= IT_COUNT || ITEMS[item].pocket == PK_KEY) return 0;
  return ITEMS[item].price / 2;
}

ShopResult sellItem(Trainer &t, uint8_t item, uint8_t qty) {
  uint32_t unit = sellPrice(item);
  if (!unit) return SHOP_NOT_SOLD;
  if (!qty || t.bag[item] < qty) return SHOP_LIMIT;
  t.bag[item] -= qty;
  earn(t, unit * qty);
  return SHOP_OK;
}

// ------------------------------------------------------------------ Celadon -

bool celadonOpen(const Trainer &t) { return t.badges & (1 << 3); }  // medalla de Erika

bool celadonSells(uint8_t item) {
  return (item >= IT_FIRE_STONE && item <= IT_LEAF_STONE) || (item >= IT_HP_UP && item <= IT_CALCIUM);
}

ShopResult celadonBuy(Trainer &t, uint8_t item, uint8_t qty) {
  if (!celadonOpen(t)) return SHOP_CLOSED;
  if (item >= IT_COUNT || !celadonSells(item)) return SHOP_NOT_SOLD;
  return buyFor(t, item, qty, ITEMS[item].price);
}

// MT de la tienda con precios de gen 1 (las de gimnasio y Battle Shop no estan)
struct TmPrice { uint8_t tm; uint16_t price; };
static const TmPrice CELADON_POOL[] = {
  {1, 3000}, {2, 2000}, {5, 3000}, {7, 2000}, {9, 3000}, {16, 2000}, {17, 3000}, {18, 3000},
  {20, 1000}, {30, 1000}, {31, 1000}, {32, 1000}, {33, 1000}, {35, 1000}, {36, 2000},
  {37, 2000}, {39, 2000},
};
#define CELADON_POOL_N (sizeof(CELADON_POOL) / sizeof(CELADON_POOL[0]))

uint16_t tmPrice(uint8_t tmNumber) {
  for (size_t i = 0; i < CELADON_POOL_N; i++)
    if (CELADON_POOL[i].tm == tmNumber) return CELADON_POOL[i].price;
  return 0;
}

void celadonTms(uint16_t week, uint8_t out[CELADON_TMS]) {
  Rng rng;
  rng.seed(0xCE1AD000u ^ (uint32_t)week * 2654435761u);
  uint8_t idx[CELADON_POOL_N];
  for (size_t i = 0; i < CELADON_POOL_N; i++) idx[i] = (uint8_t)i;
  for (int k = 0; k < CELADON_TMS; k++) {
    int j = rng.range(k, CELADON_POOL_N - 1);
    uint8_t x = idx[k]; idx[k] = idx[j]; idx[j] = x;
    out[k] = CELADON_POOL[idx[k]].tm;
  }
}

ShopResult celadonBuyTm(Trainer &t, uint16_t week, uint8_t slot) {
  if (!celadonOpen(t)) return SHOP_CLOSED;
  if (slot >= CELADON_TMS) return SHOP_NOT_SOLD;
  uint8_t tms[CELADON_TMS];
  celadonTms(week, tms);
  uint8_t tm = tms[slot];
  if (t.tms[tm - 1] >= 99) return SHOP_BAG_FULL;
  if (!spend(t, tmPrice(tm))) return SHOP_NO_MONEY;
  giveTm(t, tm);
  return SHOP_OK;
}

// -------------------------------------------------------------- Battle Shop -

const BattleShopOffer BATTLE_SHOP[NUM_BATTLE_SHOP] = {
  { BS_ITEM, IT_RARE_CANDY, 20 },  // 3 a la semana
  { BS_ITEM, IT_MOON_STONE, 15 },
  { BS_ITEM, IT_LINK_CABLE, 40 },  // evoluciona a uno que evoluciona por intercambio
  { BS_ITEM, IT_EXP_SHARE, 50 },   // una vez
  { BS_ITEM, IT_MAX_REVIVE, 12 },
  // MT exclusivas, desde 24 BP
  { BS_TM, 3, 24 }, { BS_TM, 8, 24 }, { BS_TM, 19, 24 }, { BS_TM, 44, 24 }, { BS_TM, 49, 24 },
  { BS_TM, 50, 24 }, { BS_TM, 10, 28 }, { BS_TM, 41, 28 }, { BS_TM, 43, 28 }, { BS_TM, 47, 28 },
  { BS_TM, 14, 32 }, { BS_TM, 15, 32 },
};

const uint8_t UNSOLD_TMS[NUM_UNSOLD_TMS] = { 4, 23, 40 };  // Remolino, Furia Dragon, Cabezazo

ShopResult battleShopBuy(Trainer &t, Progress &p, uint8_t idx, uint32_t now) {
  if (idx >= NUM_BATTLE_SHOP) return SHOP_NOT_SOLD;
  progressWeek(p, now);
  const BattleShopOffer &o = BATTLE_SHOP[idx];
  if (o.kind == BS_TM) {
    if (t.tms[o.id - 1] >= 99) return SHOP_BAG_FULL;
  } else {
    if (o.id == IT_RARE_CANDY && p.candiesBought >= BS_CANDIES_PER_WEEK) return SHOP_LIMIT;
    if (o.id == IT_EXP_SHARE && p.expShareBought) return SHOP_LIMIT;
    if (!bagRoom(t, o.id, 1)) return SHOP_BAG_FULL;
  }
  if (t.bp < o.bp) return SHOP_NO_BP;
  t.bp -= o.bp;
  if (o.kind == BS_TM) {
    giveTm(t, o.id);
  } else {
    addItem(t, o.id);
    if (o.id == IT_RARE_CANDY) p.candiesBought++;
    if (o.id == IT_EXP_SHARE) p.expShareBought = true;
  }
  return SHOP_OK;
}
