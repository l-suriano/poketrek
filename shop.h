#pragma once
#include <stdint.h>
#include "trainer.h"
#include "rewards.h"

// Tiendas de PokeTrek (reglas: "Economy & shops"): Poke Mart segun medallas,
// Celadon Dept. Store desde la medalla 4 con 3 MT que rotan cada semana, y la
// Battle Shop que vende con BP lo que el dinero no compra.

enum ShopResult : uint8_t { SHOP_OK, SHOP_NOT_SOLD, SHOP_NO_MONEY, SHOP_NO_BP, SHOP_LIMIT,
                            SHOP_BAG_FULL, SHOP_CLOSED };

// ---- Poke Mart (en el menu, en cualquier sitio) ----
ShopResult martBuy(Trainer &t, uint8_t item, uint8_t qty = 1);
// se vende a la mitad del precio (la Pepita: 5000). 0 = no se puede vender
uint32_t sellPrice(uint8_t item);
ShopResult sellItem(Trainer &t, uint8_t item, uint8_t qty = 1);

// ---- Celadon Dept. Store (desde la medalla de Erika) ----
#define NUM_TMS 50  // MACHINE_MOVE: 50 MT y despues las 5 MO
#define CELADON_TMS 3
bool celadonOpen(const Trainer &t);
bool celadonSells(uint8_t item);  // piedras (no la Lunar) y vitaminas
ShopResult celadonBuy(Trainer &t, uint8_t item, uint8_t qty = 1);
// las 3 MT de la semana (numeros de MT), iguales para todos esa semana
void celadonTms(uint16_t week, uint8_t out[CELADON_TMS]);
uint16_t tmPrice(uint8_t tmNumber);  // 0 = no se vende en Celadon
ShopResult celadonBuyTm(Trainer &t, uint16_t week, uint8_t slot);

// ---- Battle Shop (BP) ----
enum : uint8_t { BS_ITEM, BS_TM };
struct BattleShopOffer { uint8_t kind, id; uint8_t bp; };
#define NUM_BATTLE_SHOP 17
extern const BattleShopOffer BATTLE_SHOP[NUM_BATTLE_SHOP];
#define BS_CANDIES_PER_WEEK 3
ShopResult battleShopBuy(Trainer &t, Progress &p, uint8_t idx, uint32_t now);

// MT que no se venden en ningun sitio (premio semanal de la Featured Cup)
#define NUM_UNSOLD_TMS 3
extern const uint8_t UNSOLD_TMS[NUM_UNSOLD_TMS];
// darle una MT (numero 1..50) al jugador
void giveTm(Trainer &t, uint8_t tmNumber);
