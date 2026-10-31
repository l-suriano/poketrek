// Puente entre el motor de PokeTrek (C++) y la interfaz web del Rabbit R1.
// Usa los modulos del juego tal cual, sin tocarlos: la version del R1 es solo
// otra "placa". Cada funcion pt_* devuelve un texto JSON (o un numero) que lee
// r1/web/game.js. Se compila a WebAssembly con Emscripten (ver r1/Makefile) y
// tambien en nativo para las pruebas (r1/test_bridge.cpp).
//
// El R1 se trata como la placa sin GPS: km = pasos * 0,7 m, Supply Drop cada
// 1000 pasos afuera y la zona lejana en 1 de cada 5 eventos. "Afuera" lo elige
// el jugador (el R1 no sabe si esta en el Wi-Fi de casa).
#include "../battle.h"
#include "../gyms.h"
#include "../trainer.h"
#include "../explore.h"
#include "../rewards.h"
#include "../shop.h"
#include "../tournament.h"
#include "../dex.h"
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define API extern "C" EMSCRIPTEN_KEEPALIVE
#else
#define API extern "C"
#endif

// ------------------------------------------------------------------- JSON ---

static char J[96 * 1024];
static size_t JN;
static bool jfirst[24];
static int jdepth;

static void jreset() { JN = 0; J[0] = 0; jdepth = 0; jfirst[0] = true; }
static void jch(char c) { if (JN < sizeof(J) - 1) { J[JN++] = c; J[JN] = 0; } }
static void jraw(const char *s) { while (*s) jch(*s++); }
static void jsep() { if (!jfirst[jdepth]) jch(','); jfirst[jdepth] = false; }

// los textos del juego van en CP437 (como en la placa): pasarlos a UTF-8
static void jutf8(uint32_t cp) {
  if (cp < 0x80) jch((char)cp);
  else if (cp < 0x800) { jch((char)(0xC0 | (cp >> 6))); jch((char)(0x80 | (cp & 0x3F))); }
  else { jch((char)(0xE0 | (cp >> 12))); jch((char)(0x80 | ((cp >> 6) & 0x3F))); jch((char)(0x80 | (cp & 0x3F))); }
}
static uint32_t cp437(uint8_t c) {
  static const uint16_t HI[48] = {  // 0x80..0xAF
    0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, 0xEA, 0xEB, 0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5,
    0xC9, 0xE6, 0xC6, 0xF4, 0xF6, 0xF2, 0xFB, 0xF9, 0xFF, 0xD6, 0xDC, 0xA2, 0xA3, 0xA5, 0x20A7, 0x192,
    0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA, 0xBF, 0x2310, 0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB,
  };
  if (c < 0x80) return c;
  return c < 0xB0 ? HI[c - 0x80] : '?';
}
static void jtext(const char *s) {
  jch('"');
  for (; s && *s; s++) {
    uint8_t c = (uint8_t)*s;
    if (c == '"' || c == '\\') { jch('\\'); jch((char)c); }
    else if (c < 0x20) jch(' ');
    else jutf8(cp437(c));
  }
  jch('"');
}
static void jkey(const char *k) { jsep(); jtext(k); jch(':'); }
static void jobj(const char *k = nullptr) { if (k) jkey(k); else jsep(); jch('{'); jfirst[++jdepth] = true; }
static void jarr(const char *k = nullptr) { if (k) jkey(k); else jsep(); jch('['); jfirst[++jdepth] = true; }
static void jend(char c) { jch(c); jdepth--; }
static void jnum(long v) { char b[24]; snprintf(b, sizeof(b), "%ld", v); jraw(b); }
static void ji(const char *k, long v) { jkey(k); jnum(v); }
static void js(const char *k, const char *v) { jkey(k); jtext(v); }
static void jb(const char *k, bool v) { jkey(k); jraw(v ? "true" : "false"); }
static void jvi(long v) { jsep(); jnum(v); }
static const char *jdone() { return J; }

// ------------------------------------------------------------------ estado --

// lo propio de la version del R1, guardado junto a la partida
struct Ext {
  uint16_t lastDay = 0xFFFF;
  uint32_t healAt = 0;          // fin de la curacion de 30 min fuera de casa
  uint16_t pendingEvents = 0;   // eventos afuera que esperan
  uint16_t pendingDrops = 0;    // Supply Drops que esperan
  uint8_t lang = LANG_EN;
  bool outside = false;
  bool started = false;
  uint8_t leagueIdx = 0;        // siguiente rival de la Liga en la corrida actual
  bool leagueActive = false;
};

static Trainer TR;
static Explore EX;
static Progress PG;
static Ext XT;
static Rng RNG;
static uint32_t NOW = 1793000000u;

enum BKind : uint8_t { BK_NONE, BK_WILD, BK_LEGEND, BK_TRAINER, BK_GYM, BK_ELITE, BK_TOUR, BK_RIVAL };
static Battle BT;
static uint8_t bKind = BK_NONE;
static uint8_t bGym, bAce, bLast;
static uint32_t bPrize;
static uint8_t bPartMask, bFought;  // participantes contra el rival activo / en todo el combate
static bool bFaintedAny, bCaught;
static uint8_t bItemsUsed;
static ExpBonus bBonus;
static uint8_t bFoeActive;

static Encounter ENC;
static bool encActive = false;
static Tournament TOUR;
static bool tourActive = false;
static Loot LOOT;

// cosas que esperan al jugador tras un combate: aprender un movimiento o evolucionar
enum PendKind : uint8_t { PD_LEARN, PD_EVOLVE };
struct Pend { uint8_t kind, inBox, idx, a; };
static Pend PQ[24];
static uint8_t nPend;

static OwnedMon &monAt(uint8_t inBox, uint8_t idx) { return inBox ? TR.box[idx] : TR.party[idx]; }

// PC de 12 cajas de 20 (240, como en gen 1): las 3 primeras son el PC del motor
// (TR.box, 60) y las otras 9 viven aqui. La lista completa es TR.box seguido de EXTRA.
#define BOX_PAGE 20
#define EXTRA_MAX 180
struct Ext3 {
  uint16_t count = 0;
  OwnedMon mon[EXTRA_MAX];
};
static Ext3 EXTRA;

static int boxTotal() { return TR.boxCount + EXTRA.count; }
static OwnedMon *boxAt(int g) {
  if (g < 0) return nullptr;
  if (g < TR.boxCount) return &TR.box[g];
  g -= TR.boxCount;
  return g < EXTRA.count ? &EXTRA.mon[g] : nullptr;
}
static bool boxFull() { return TR.boxCount >= BOX_MAX && EXTRA.count >= EXTRA_MAX; }
static bool teamLocked();

// deja sitio en el PC del motor pasando los ultimos a las cajas extra (el orden se mantiene)
static void spillBox() {
  if (teamLocked() || TR.boxCount <= BOX_MAX - BOX_PAGE) return;
  int k = TR.boxCount - (BOX_MAX - 2 * BOX_PAGE);
  if (EXTRA.count + k > EXTRA_MAX) k = EXTRA_MAX - EXTRA.count;
  if (k <= 0) return;
  memmove(EXTRA.mon + k, EXTRA.mon, EXTRA.count * sizeof(OwnedMon));
  for (int i = 0; i < k; i++) EXTRA.mon[i] = TR.box[TR.boxCount - k + i];
  EXTRA.count += k;
  TR.boxCount -= k;
}

// lo que se guarda aparte: el torneo en curso y lo pendiente de aprender o evolucionar
struct Ext2 {
  Tournament tour;
  bool tourActive = false;
  uint8_t nPend = 0;
  Pend pq[24];
};

static void pushPend(uint8_t kind, uint8_t inBox, uint8_t idx, uint8_t a) {
  if (kind == PD_EVOLVE)  // una sola evolucion pendiente por Pokemon: la ultima
    for (int i = 0; i < nPend; i++)
      if (PQ[i].kind == PD_EVOLVE && PQ[i].inBox == inBox && PQ[i].idx == idx) { PQ[i].a = a; return; }
  if (nPend < sizeof(PQ) / sizeof(PQ[0])) PQ[nPend++] = { kind, inBox, idx, a };
}

static bool isNight() {
  uint32_t h = (NOW % 86400u) / 3600u;
  return h >= 20 || h < 6;
}

// ------------------------------------------------------------- serializar --

static void jmoves(const uint8_t *moves, const uint8_t *pp) {
  jarr("moves");
  for (int k = 0; k < 4; k++) {
    if (!moves[k]) continue;
    jobj();
    ji("id", moves[k]);
    js("name", moveName(moves[k]));
    ji("type", MOVE_TBL[moves[k]].type);
    ji("pp", pp[k]);
    ji("max", MOVE_TBL[moves[k]].pp);
    ji("pow", MOVE_TBL[moves[k]].power);
    jend('}');
  }
  jend(']');
}

static void jowned(const OwnedMon &m, bool full) {
  jobj();
  ji("sp", m.species);
  js("name", dexName(m.species));
  ji("lv", m.level);
  ji("hp", m.hp);
  ji("max", ownedMaxHp(m));
  ji("st", m.status);
  ji("t1", BSPECIES[m.species].type1);
  ji("t2", BSPECIES[m.species].type2);
  ji("hd", BSPECIES[m.species].heightDm);
  jb("shiny", m.flags & OM_SHINY);
  jb("partner", m.flags & OM_PARTNER);
  ji("trust", m.trust);
  if (full) {
    uint8_t g = BSPECIES[m.species].growth;
    ji("exp", m.exp);
    ji("expLv", expForLevel(g, m.level));
    ji("expNext", m.level < 100 ? expForLevel(g, m.level + 1) : m.exp);
    BattleMon b;
    toBattle(m, b);
    ji("atk", b.atk); ji("def", b.def); ji("spe", b.spe); ji("spc", b.spc);
    jmoves(m.moves, m.pp);
  }
  jend('}');
}

static void jbattlemon(const BattleMon &m, bool moves) {
  jobj();
  ji("sp", m.species);
  js("name", dexName(m.species));
  ji("lv", m.level);
  ji("hp", m.hp);
  ji("max", m.maxHp);
  ji("st", m.status);
  ji("t1", m.type1);
  ji("t2", m.type2);
  ji("hd", BSPECIES[m.species].heightDm);
  if (moves) jmoves(m.moves, m.pp);
  jend('}');
}

static void jencounter() {
  jobj();
  static const char *KIND[] = { "nothing", "wild", "trainer", "item", "legend" };
  js("kind", encActive ? KIND[ENC.kind] : "none");
  if (encActive) {
    ji("zone", ENC.zone);
    if (ENC.kind == EN_WILD || ENC.kind == EN_LEGEND) {
      jkey("wild"); jfirst[jdepth] = true; jbattlemon(ENC.wild, false);
      jb("shiny", ENC.shiny);
      ji("expires", ENC.expiresAt);
    } else if (ENC.kind == EN_TRAINER) {
      const TrainerClass &c = TRAINER_CLASSES[ENC.trainer.cls];
      js("cls", gLang == LANG_ES ? c.es : c.en);
      ji("clsId", ENC.trainer.cls);  // para el sprite del entrenador
      ji("prize", ENC.trainer.prize);
      jarr("party");
      for (int i = 0; i < ENC.trainer.count; i++) { jobj(); ji("sp", ENC.trainer.party[i].species); ji("lv", ENC.trainer.party[i].level); jend('}'); }
      jend(']');
    } else if (ENC.kind == EN_ITEM) {
      ji("item", ENC.item);
      js("itemName", ENC.item == ITEM_BERRY ? (gLang == LANG_ES ? "Bayas" : "Berries") : itemName(ENC.item));
      ji("n", ENC.itemCount);
    }
  }
  jend('}');
}

static void jloot(const Loot &l) {
  jobj("loot");
  jarr("items");
  for (int i = 0; i < l.count; i++) { jobj(); ji("id", l.item[i]); js("name", itemName(l.item[i])); ji("n", l.n[i]); jend('}'); }
  jend(']');
  ji("berries", l.berries);
  ji("egg", l.eggTier);
  jb("merchant", l.merchant);
  if (l.merchant) {
    jarr("offer");
    for (int i = 0; i < MERCHANT_ITEMS; i++) { jobj(); ji("id", l.offer[i].item); js("name", itemName(l.offer[i].item)); ji("price", l.offer[i].price); jend('}'); }
    jend(']');
  }
  jend('}');
}

// ------------------------------------------------------------- arrancar ----

API int pt_buf_size() { return 64 * 1024; }
static uint8_t SAVEBUF[64 * 1024];
API uint8_t *pt_buf() { return SAVEBUF; }

API void pt_init(int lang, uint32_t now, uint32_t seed) {
  gLang = (Lang)lang;
  NOW = now;
  RNG.seed(seed);
}

API void pt_set_lang(int lang) { gLang = (Lang)lang; XT.lang = (uint8_t)lang; }

API void pt_new(int starter) {
  TR = Trainer();
  exploreInit(EX);
  progressInit(PG);
  XT = Ext();
  XT.lang = (uint8_t)gLang;
  XT.started = true;
  XT.lastDay = dayIndex(NOW);
  EXTRA = Ext3();
  OwnedMon m;
  ownedInit(m, (uint8_t)starter, 5, RNG);
  m.flags |= OM_PARTNER;
  addOwned(TR, m);
  dexMark(TR.seen, (uint8_t)starter);
  dexMark(TR.caught, (uint8_t)starter);
  addItem(TR, IT_POKE_BALL, 5);  // reglas: 5 Poke Balls, 3 Pociones y 3000
  addItem(TR, IT_POTION, 3);
  TR.money = 3000;
  encActive = false;
  bKind = BK_NONE;
  tourActive = false;
  nPend = 0;
}

static void put32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static uint32_t get32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

// guarda todo en pt_buf(): [len][Trainer][len][Explore][len][Progress][len][Ext]
API int pt_save() {
  size_t off = 0;
  size_t n;
  n = trainerSave(TR, SAVEBUF + off + 4, sizeof(SAVEBUF) - off - 4); put32(SAVEBUF + off, (uint32_t)n); off += 4 + n;
  n = exploreSave(EX, SAVEBUF + off + 4, sizeof(SAVEBUF) - off - 4); put32(SAVEBUF + off, (uint32_t)n); off += 4 + n;
  n = progressSave(PG, SAVEBUF + off + 4, sizeof(SAVEBUF) - off - 4); put32(SAVEBUF + off, (uint32_t)n); off += 4 + n;
  n = saveBlob("PTX1", 1, &XT, sizeof(XT), SAVEBUF + off + 4, sizeof(SAVEBUF) - off - 4); put32(SAVEBUF + off, (uint32_t)n); off += 4 + n;
  // seccion extra (las partidas viejas no la tienen y siguen cargando)
  Ext2 x2;
  x2.tour = TOUR; x2.tourActive = tourActive; x2.nPend = nPend;
  memcpy(x2.pq, PQ, sizeof(PQ));
  n = saveBlob("PTX2", 1, &x2, sizeof(x2), SAVEBUF + off + 4, sizeof(SAVEBUF) - off - 4); put32(SAVEBUF + off, (uint32_t)n); off += 4 + n;
  n = saveBlob("PTX3", 1, &EXTRA, sizeof(EXTRA), SAVEBUF + off + 4, sizeof(SAVEBUF) - off - 4); put32(SAVEBUF + off, (uint32_t)n); off += 4 + n;
  return (int)off;
}

API int pt_load(int len) {
  size_t off = 0;
  Trainer t; Explore e; Progress p; Ext x;
  auto next = [&](size_t &n) -> const uint8_t * {
    if (off + 4 > (size_t)len) return nullptr;
    n = get32(SAVEBUF + off);
    if (off + 4 + n > (size_t)len) return nullptr;
    const uint8_t *q = SAVEBUF + off + 4;
    off += 4 + n;
    return q;
  };
  size_t n;
  const uint8_t *q;
  if (!(q = next(n)) || !trainerLoad(t, q, n)) return 0;
  if (!(q = next(n)) || !exploreLoad(e, q, n)) return 0;
  if (!(q = next(n)) || !progressLoad(p, q, n)) return 0;
  if (!(q = next(n)) || !loadBlob("PTX1", 1, &x, sizeof(x), q, n)) return 0;
  TR = t; EX = e; PG = p; XT = x;
  gLang = (Lang)XT.lang;
  encActive = false;
  bKind = BK_NONE;
  tourActive = false;
  nPend = 0;
  Ext2 x2;
  if ((q = next(n)) && loadBlob("PTX2", 1, &x2, sizeof(x2), q, n)) {
    TOUR = x2.tour; tourActive = x2.tourActive;
    nPend = x2.nPend <= sizeof(PQ) / sizeof(PQ[0]) ? x2.nPend : 0;
    memcpy(PQ, x2.pq, sizeof(PQ));
  }
  EXTRA = Ext3();
  static Ext3 x3;
  if ((q = next(n)) && loadBlob("PTX3", 1, &x3, sizeof(x3), q, n) && x3.count <= EXTRA_MAX) EXTRA = x3;
  return 1;
}

API int pt_started() { return XT.started; }

// el almacenamiento del R1 (creationStorage) pide texto en base64
static const char B64C[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char B64[sizeof(SAVEBUF) / 3 * 4 + 8];

API const char *pt_save_b64() {
  int n = pt_save();
  size_t o = 0;
  for (int i = 0; i < n; i += 3) {
    uint32_t v = (uint32_t)SAVEBUF[i] << 16 | (i + 1 < n ? SAVEBUF[i + 1] << 8 : 0) | (i + 2 < n ? SAVEBUF[i + 2] : 0);
    B64[o++] = B64C[v >> 18 & 63];
    B64[o++] = B64C[v >> 12 & 63];
    B64[o++] = i + 1 < n ? B64C[v >> 6 & 63] : '=';
    B64[o++] = i + 2 < n ? B64C[v & 63] : '=';
  }
  B64[o] = 0;
  return B64;
}

API int pt_load_b64(const char *s) {
  uint32_t v = 0;
  int bits = 0;
  size_t n = 0;
  for (; *s && *s != '='; s++) {
    const char *p = strchr(B64C, *s);
    if (!p) continue;
    v = v << 6 | (uint32_t)(p - B64C);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      if (n >= sizeof(SAVEBUF)) return 0;
      SAVEBUF[n++] = (uint8_t)(v >> bits);
    }
  }
  return pt_load((int)n);
}

// llamar antes de cada accion: dia nuevo, curacion pendiente, semana
API void pt_tick(uint32_t now) {
  NOW = now;
  uint16_t day = dayIndex(now);
  if (XT.lastDay != day) { trustNewDay(TR); XT.lastDay = day; }
  if (XT.healAt && now >= XT.healAt && bKind == BK_NONE) { healAll(TR); XT.healAt = 0; }
  progressWeek(PG, now);
  if (encActive && ENC.expiresAt && now >= ENC.expiresAt && bKind == BK_NONE) encActive = false;
}

API void pt_set_outside(int on) { XT.outside = on != 0; }

// ------------------------------------------------------------------ estado --

static bool gymOpen(uint8_t g) {
  if (g >= NUM_GYMS) return false;
  if (g > 0 && !(TR.badges & (1 << (g - 1)))) return false;
  return EX.meters / 1000 >= GYMS[g].kmToUnlock;
}
static bool leagueOpen() { return badgeCount(TR) == NUM_BADGES && EX.meters / 1000 >= 80; }

API const char *pt_state() {
  jreset();
  jobj();
  ji("money", TR.money);
  ji("bp", TR.bp);
  ji("badges", TR.badges);
  jb("champion", TR.champion);
  ji("ee", EX.ee);
  ji("eeMax", EE_MAX);
  ji("steps", EX.steps);
  ji("meters", EX.meters);
  ji("toEvent", EX.stepsToEvent);
  ji("toDrop", EX.stepsToDrop);
  ji("events", XT.pendingEvents);
  ji("drops", XT.pendingDrops);
  jb("outside", XT.outside);
  jb("night", isNight());
  ji("lang", gLang);
  ji("healAt", XT.healAt);
  ji("bl", partyBaseLevel(TR));
  jb("incense", incenseActive(EX, NOW));
  ji("incenseUntil", EX.incenseUntil);
  ji("seen", dexCount(TR.seen));
  ji("caught", dexCount(TR.caught));
  ji("dcDays", PG.dcDays);
  jb("dcPlayed", dailyPlayedToday(PG, NOW));
  ji("boxCount", boxTotal());
  ji("boxMax", BOX_MAX + EXTRA_MAX);
  ji("pending", nPend);
  jb("encounter", encActive);
  jb("leagueOpen", leagueOpen());
  jb("leagueActive", XT.leagueActive);
  ji("leagueIdx", XT.leagueIdx);
  jarr("party");
  for (int i = 0; i < TR.partyCount; i++) jowned(TR.party[i], true);
  jend(']');
  jarr("bag");
  for (int i = 0; i < IT_COUNT; i++) {
    if (!TR.bag[i]) continue;
    jobj(); ji("id", i); js("name", itemName((uint8_t)i)); ji("n", TR.bag[i]); ji("pocket", ITEMS[i].pocket); jend('}');
  }
  jend(']');
  jarr("tms");
  for (int i = 0; i < NUM_MACHINES; i++) {
    if (!TR.tms[i]) continue;
    jobj(); ji("num", i + 1); ji("move", MACHINE_MOVE[i]); js("name", moveName(MACHINE_MOVE[i])); ji("n", TR.tms[i]); jend('}');
  }
  jend(']');
  jarr("eggs");
  for (int i = 0; i < MAX_EGGS; i++) {
    const Egg &e = EX.eggs[i];
    jobj();
    ji("tier", e.species ? e.tier : 0);
    ji("walked", e.walkedM);
    ji("goal", e.species ? eggKm(e.tier) * 1000 : 0);
    jend('}');
  }
  jend(']');
  jend('}');
  return jdone();
}

API const char *pt_box() {
  jreset();
  jarr();
  for (int i = 0; i < TR.boxCount; i++) jowned(TR.box[i], false);
  jend(']');
  return jdone();
}

// caja `page` (0..11): 20 Pokemon de la lista completa del PC
API const char *pt_box_page(int page) {
  jreset();
  jobj();
  ji("page", page);
  ji("pages", (BOX_MAX + EXTRA_MAX) / BOX_PAGE);
  ji("total", boxTotal());
  jarr("mons");
  for (int i = 0; i < BOX_PAGE; i++) {
    OwnedMon *m = boxAt(page * BOX_PAGE + i);
    if (!m) break;
    jowned(*m, false);
  }
  jend(']');
  jend('}');
  return jdone();
}

API const char *pt_box_mon(int g) {
  jreset();
  OwnedMon *m = boxAt(g);
  if (!m) jraw("null"); else jowned(*m, true);
  return jdone();
}

// sacar al equipo (g = posicion en la lista completa)
API int pt_box_withdraw(int g) {
  if (teamLocked() || TR.partyCount >= PARTY_MAX) return 0;
  if (g < TR.boxCount) return withdrawFromBox(TR, (uint8_t)g);
  int e = g - TR.boxCount;
  if (e < 0 || e >= EXTRA.count) return 0;
  TR.party[TR.partyCount++] = EXTRA.mon[e];
  memmove(EXTRA.mon + e, EXTRA.mon + e + 1, (EXTRA.count - e - 1) * sizeof(OwnedMon));
  EXTRA.count--;
  return 1;
}

API int pt_box_release(int g) {
  if (teamLocked()) return 0;
  if (g < TR.boxCount) {
    for (int i = g; i + 1 < TR.boxCount; i++) TR.box[i] = TR.box[i + 1];
    TR.boxCount--;
    return 1;
  }
  int e = g - TR.boxCount;
  if (e < 0 || e >= EXTRA.count) return 0;
  memmove(EXTRA.mon + e, EXTRA.mon + e + 1, (EXTRA.count - e - 1) * sizeof(OwnedMon));
  EXTRA.count--;
  return 1;
}

API const char *pt_mon(int inBox, int idx) {
  jreset();
  if ((inBox && idx >= TR.boxCount) || (!inBox && idx >= TR.partyCount)) { jraw("null"); return jdone(); }
  jowned(monAt((uint8_t)inBox, (uint8_t)idx), true);
  return jdone();
}

// pokedex: un caracter por especie (0 nada, 1 visto, 2 atrapado)
API const char *pt_dex() {
  static char d[160];
  for (int s = 1; s <= 151; s++) d[s - 1] = dexHas(TR.caught, (uint8_t)s) ? '2' : dexHas(TR.seen, (uint8_t)s) ? '1' : '0';
  d[151] = 0;
  return d;
}

API const char *pt_species_name(int sp) {
  jreset();
  jtext(dexName((int16_t)sp));
  return jdone();
}

// ---------------------------------------------------------------- caminar --

API const char *pt_walk(int steps, int outside) {
  XT.outside = outside != 0;
  WalkInput in;
  in.steps = (uint16_t)(steps > 60000 ? 60000 : steps);
  in.outside = XT.outside;
  in.gps = false;
  in.now = NOW;
  WalkResult r = exploreWalk(EX, TR, in);
  XT.pendingEvents += r.events;
  XT.pendingDrops += r.drops;
  jreset();
  jobj();
  ji("ee", r.ee); ji("events", r.events); ji("drops", r.drops); ji("km", r.km);
  ji("eggsReady", r.eggsReady); ji("candies", r.candies);
  jend('}');
  return jdone();
}

// de noche salen mas Pokemon nocturnos (solo en la version del R1, sin tocar explore.cpp)
static const uint8_t NIGHT_SPECIES[] = { 41, 43, 35, 92, 52, 96, 48, 39, 19 };

static void nightReroll(int route, const uint8_t *seenBefore) {
  if (ENC.kind != EN_WILD || !isNight() || !RNG.chance(35)) return;
  uint8_t cand[sizeof(NIGHT_SPECIES)];
  uint8_t n = 0;
  for (uint8_t s : NIGHT_SPECIES) {
    if (route >= 0) {
      bool inPool = false;
      for (int i = 0; i < ROUTE_POOL && ROUTES[route].pool[i].weight; i++) inPool |= ROUTES[route].pool[i].species == s;
      if (!inPool) continue;
    } else if (ENC.zone == ZONE_NEAR && WILD_TIER[s] == WT_MID && !RNG.chance(15)) {
      continue;  // en la zona cercana las medias siguen siendo pocas
    }
    cand[n++] = s;
  }
  if (!n) return;
  memcpy(TR.seen, seenBefore, DEX_BYTES);  // el que se iba a ver no llego a salir
  exploreWild(ENC.wild, cand[RNG.range(0, n - 1)], ENC.wild.level, RNG);
  dexMark(TR.seen, ENC.wild.species);
}

// siguiente evento afuera que espera (primero los Supply Drops)
API const char *pt_next_event() {
  jreset();
  if (XT.pendingDrops) {
    XT.pendingDrops--;
    exploreLoot(EX, TR, RNG, LOOT);
    jobj(); js("kind", "drop"); jloot(LOOT); jend('}');
    return jdone();
  }
  if (!XT.pendingEvents) { jraw("{\"kind\":\"none\"}"); return jdone(); }
  XT.pendingEvents--;
  uint8_t seen[DEX_BYTES];
  memcpy(seen, TR.seen, DEX_BYTES);
  uint8_t zone = exploreZone(EX, false, 0, 0);
  ENC = exploreOutdoorEvent(EX, TR, RNG, zone, NOW, wildShinyDen(PG));
  encActive = ENC.kind == EN_WILD || ENC.kind == EN_TRAINER;
  nightReroll(-1, seen);
  jencounter();
  return jdone();
}

API const char *pt_routes() {
  jreset();
  jarr();
  for (int r = 0; r < NUM_ROUTES; r++) {
    jobj();
    js("name", routeName((uint8_t)r));
    jb("open", routeOpen(TR, (uint8_t)r));
    ji("badges", ROUTES[r].badges);
    ji("legend", ROUTES[r].legend);
    jend('}');
  }
  jend(']');
  return jdone();
}

API const char *pt_search(int route) {
  jreset();
  uint8_t seen[DEX_BYTES];
  memcpy(seen, TR.seen, DEX_BYTES);
  if (!exploreSearchRoute(EX, TR, RNG, (uint8_t)route, NOW, ENC, wildShinyDen(PG))) {
    jraw("{\"kind\":\"error\"}");
    return jdone();
  }
  encActive = ENC.kind == EN_WILD || ENC.kind == EN_TRAINER || ENC.kind == EN_LEGEND;
  nightReroll(route, seen);
  jencounter();
  return jdone();
}

API const char *pt_encounter() { jreset(); jencounter(); return jdone(); }
API void pt_leave_encounter() {
  if (encActive && ENC.kind == EN_LEGEND) exploreLegendResult(EX, ENC.legend, false, NOW);
  encActive = false;
}

API int pt_incense_start(int route) { return incenseStart(EX, TR, (uint8_t)route, NOW); }

API const char *pt_incense_tick() {
  jreset();
  Encounter e;
  if (bKind != BK_NONE || encActive || !incenseTick(EX, TR, RNG, NOW, e, wildShinyDen(PG))) {
    jraw("{\"kind\":\"none\"}");
    return jdone();
  }
  ENC = e;
  encActive = true;
  jencounter();
  return jdone();
}

API int pt_merchant_buy(int i) { return merchantBuy(TR, LOOT, (uint8_t)i); }

// ---------------------------------------------------------------- captura --

static int ballOk(int ball) {
  return (ball == IT_POKE_BALL || ball == IT_GREAT_BALL || ball == IT_ULTRA_BALL || ball == IT_MASTER_BALL) && TR.bag[ball];
}

// lanzamiento fuera de combate (como en GO): hit = la mira estaba sobre el Pokemon
API const char *pt_throw(int ball, int throwX10, int hit) {
  jreset();
  if (!encActive || (ENC.kind != EN_WILD && ENC.kind != EN_LEGEND) || bKind != BK_NONE) { jraw("{\"res\":\"none\"}"); return jdone(); }
  if (!ballOk(ball)) { jraw("{\"res\":\"noball\"}"); return jdone(); }
  spillBox();
  if (TR.partyCount >= PARTY_MAX && boxFull()) { jraw("{\"res\":\"boxfull\"}"); return jdone(); }
  TR.bag[ball]--;
  jobj();
  if (!hit) {
    js("res", "miss");  // la bola se pierde
  } else {
    uint8_t shakes = 0;
    if (catchAttempt(RNG, ENC.wild, (uint8_t)ball, (uint8_t)throwX10, &shakes)) {
      bool ok = addCaught(TR, ENC.wild, RNG, ENC.shiny);
      if (ENC.kind == EN_LEGEND) exploreLegendResult(EX, ENC.legend, true, NOW);
      encActive = false;
      js("res", ok ? "caught" : "full");
    } else if (RNG.chance(BSPECIES[ENC.wild.species].flee)) {
      // reglas: tras un lanzamiento fallido fuera de combate puede huir
      if (ENC.kind == EN_LEGEND) exploreLegendResult(EX, ENC.legend, false, NOW);
      encActive = false;
      js("res", "fled");
    } else {
      js("res", "broke");
    }
    ji("shakes", shakes);
  }
  ji("left", TR.bag[ball]);
  jend('}');
  return jdone();
}

API int pt_catch_chance(int ball, int throwX10) {
  if (!encActive) return 0;
  const BattleMon &w = bKind != BK_NONE ? activeMon(BT, 1) : ENC.wild;
  return catchChancePermille(w, ballMultX10((uint8_t)ball), (uint8_t)throwX10);
}

// ---------------------------------------------------------------- combate --

static bool tourBattle() { return bKind == BK_TOUR; }

static OwnedMon &ownedFor(uint8_t i) {
  if (tourBattle()) { const TourPick &p = TOUR.picks[i]; return monAt(p.inBox, p.idx); }
  return TR.party[i];
}
static void ownedLoc(uint8_t i, uint8_t &inBox, uint8_t &idx) {
  if (tourBattle()) { inBox = TOUR.picks[i].inBox; idx = TOUR.picks[i].idx; }
  else { inBox = 0; idx = i; }
}

static void buildPlayerSide() {
  BattleSide &s = BT.side[0];
  s = BattleSide();
  for (int i = 0; i < TR.partyCount; i++) toBattle(TR.party[i], s.party[i]);
  s.count = TR.partyCount;
  for (int i = 0; i < s.count; i++) if (s.party[i].hp) { s.active = (uint8_t)i; break; }
}

static void startCommon(uint8_t kind) {
  bKind = kind;
  bPartMask = bFought = (uint8_t)(1 << BT.side[0].active);
  bFaintedAny = bCaught = false;
  bItemsUsed = 0;
  bFoeActive = BT.side[1].active;
}

static void jview();

// kind: 1 encuentro actual (salvaje, legendario o entrenador), 2 gimnasio arg,
// 3 siguiente rival de la Liga, 4 ronda del torneo
API const char *pt_battle_start(int kind, int arg) {
  jreset();
  BT = Battle();
  battleInit(BT, RNG.next(), false);
  bBonus = { false, false, false, false };
  if (kind == 1) {
    if (!anyAlive(TR)) { jraw("{\"error\":\"fainted\"}"); return jdone(); }
    if (!encActive) { jraw("{\"error\":\"gone\"}"); return jdone(); }
    buildPlayerSide();
    BattleSide &f = BT.side[1];
    if (ENC.kind == EN_TRAINER) {
      routeTrainerToSide(ENC.trainer, f);
      bPrize = ENC.trainer.prize;
      bBonus.trainer = true;
      startCommon(BK_TRAINER);
    } else {
      BT.wild = true;
      f = BattleSide();
      f.party[0] = ENC.wild;
      f.count = 1;
      f.aiTier = 1;
      startCommon(ENC.kind == EN_LEGEND ? BK_LEGEND : BK_WILD);
    }
  } else if (kind == 2) {
    if (!anyAlive(TR)) { jraw("{\"error\":\"fainted\"}"); return jdone(); }
    if (!gymOpen((uint8_t)arg)) { jraw("{\"error\":1}"); return jdone(); }
    buildPlayerSide();
    buildGymTeam(BT.side[1], (uint8_t)arg, BT.side[0], RNG);
    bGym = (uint8_t)arg;
    bAce = 0;
    for (int i = 0; i < BT.side[1].count; i++) if (BT.side[1].party[i].level > bAce) bAce = BT.side[1].party[i].level;
    bLast = BT.side[1].party[BT.side[1].count - 1].level;
    bool first = !(TR.badges & (1 << arg));
    bBonus = { true, first, false, !first && TR.champion };
    startCommon(BK_GYM);
  } else if (kind == 3) {
    if (!anyAlive(TR)) { jraw("{\"error\":\"fainted\"}"); return jdone(); }
    if (!leagueOpen()) { jraw("{\"error\":1}"); return jdone(); }
    if (!XT.leagueActive) { XT.leagueActive = true; XT.leagueIdx = 0; }
    buildPlayerSide();
    buildEliteTeam(BT.side[1], XT.leagueIdx, BT.side[0], RNG);
    bBonus = { true, !(PG.eliteBeaten & (1 << XT.leagueIdx)), false, false };
    startCommon(BK_ELITE);
  } else if (kind == 5) {
    // rival (Blue) en los momentos clave del viaje: arg = etapa 0..4. Su equipo crece
    // de 2 a 6 y lleva el inicial que gana al tuyo, como en el original.
    static const uint8_t SIZE[5] = { 2, 3, 4, 5, 6 };
    static const int8_t PCT[5] = { -5, -6, -8, -10, -12 };  // afinar con sim/sim
    static const uint8_t AI[5] = { 2, 3, 3, 4, 4 };
    if (!anyAlive(TR)) { jraw("{\"error\":\"fainted\"}"); return jdone(); }
    if (arg < 0 || arg > 4) { jraw("{\"error\":1}"); return jdone(); }
    buildPlayerSide();
    BattleSide full;
    buildEliteTeam(full, ELITE_CHAMPION, BT.side[0], RNG, (int8_t)(PCT[arg] - CHAMPION_EXTRA_PCT));
    BattleSide &f = BT.side[1];
    f = full;
    uint8_t n = SIZE[arg] < full.count ? SIZE[arg] : full.count;
    for (int i = 0; i < n; i++) f.party[i] = full.party[full.count - n + i];  // los mas fuertes y el inicial
    f.count = n;
    f.aiTier = AI[arg];
    f.potions = arg < 2 ? 1 : 2;
    bPrize = prizeMoney(35, f.party[n - 1].level);
    bBonus = { true, false, false, false };
    startCommon(BK_RIVAL);
  } else if (kind == 4) {
    if (!tourActive || tourOver(TOUR)) { jraw("{\"error\":1}"); return jdone(); }
    bKind = BK_TOUR;  // ownedFor mira las inscripciones
    tourPlayerSide(TOUR, TR, BT.side[0]);
    tourOpponent(TOUR, BT.side[0], BT.side[1]);
    bBonus = tourExpBonus(TOUR);
    startCommon(BK_TOUR);
  } else {
    jraw("{\"error\":1}");
    return jdone();
  }
  jview();
  return jdone();
}

static bool canThrow() { return bKind == BK_WILD || bKind == BK_LEGEND; }

static void jview() {
  jobj();
  static const char *KIND[] = { "none", "wild", "legend", "trainer", "gym", "elite", "tour", "rival" };
  js("kind", KIND[bKind]);
  ji("turn", BT.turn);
  ji("outcome", bCaught ? 9 : BT.outcome);
  jb("needSwitch", BT.outcome == OUT_NONE && battleNeedsSwitch(BT, 0));
  jb("canRun", BT.wild);
  jb("canThrow", canThrow());
  ji("itemsLeft", tourBattle() ? (int)tourItemsAllowed(TOUR) - bItemsUsed : 99);
  if (bKind == BK_GYM) js("foeName", GYMS[bGym].leader);
  else if (bKind == BK_ELITE) js("foeName", ELITE[XT.leagueIdx].name);
  else if (bKind == BK_RIVAL) js("foeName", ELITE[ELITE_CHAMPION].name);
  else if (bKind == BK_TRAINER) js("foeName", gLang == LANG_ES ? TRAINER_CLASSES[ENC.trainer.cls].es : TRAINER_CLASSES[ENC.trainer.cls].en);
  else if (bKind == BK_TOUR) {
    TourRival r = tourRival(TOUR);
    js("foeName", r.special == SPC_GYM ? GYMS[r.idx].leader : r.special != SPC_NONE ? ELITE[r.idx].name
                  : (gLang == LANG_ES ? TRAINER_CLASSES[r.idx].es : TRAINER_CLASSES[r.idx].en));
  }
  if (canThrow()) jb("shiny", ENC.shiny);
  // quien es el rival, para su sprite: clase de entrenador, lider o miembro de la Liga
  if (bKind == BK_TRAINER) ji("tclass", ENC.trainer.cls);
  else if (bKind == BK_GYM) ji("tgym", bGym);
  else if (bKind == BK_ELITE) ji("telite", XT.leagueIdx);
  else if (bKind == BK_RIVAL) ji("telite", ELITE_CHAMPION);
  else if (bKind == BK_TOUR) {
    TourRival r = tourRival(TOUR);
    if (r.special == SPC_GYM) ji("tgym", r.idx);
    else if (r.special != SPC_NONE) ji("telite", r.idx);
    else ji("tclass", r.idx);
  }
  jobj("me");
  ji("active", BT.side[0].active);
  jarr("party");
  for (int i = 0; i < BT.side[0].count; i++) jbattlemon(BT.side[0].party[i], true);
  jend(']');
  jend('}');
  jobj("foe");
  ji("active", BT.side[1].active);
  ji("count", BT.side[1].count);
  int alive = 0;
  for (int i = 0; i < BT.side[1].count; i++) alive += BT.side[1].party[i].hp > 0;
  ji("alive", alive);
  jkey("mon"); jfirst[jdepth] = true; jbattlemon(activeMon(BT, 1), false);
  jend('}');
  jend('}');
}

API const char *pt_battle_view() { jreset(); jview(); return jdone(); }

// XP al debilitar al rival activo: se reparte entre los que pelearon contra el
static void awardExp(uint8_t foeIdx) {
  const BattleMon &foe = BT.side[1].party[foeIdx];
  BattleSide &me = BT.side[0];
  uint8_t parts = 0;
  for (int i = 0; i < me.count; i++) if ((bPartMask >> i & 1) && me.party[i].hp) parts |= (uint8_t)(1 << i);
  int n = __builtin_popcount(parts);
  bool share = !tourBattle() && TR.bag[IT_EXP_SHARE];
  for (int i = 0; i < me.count; i++) {
    bool part = parts >> i & 1;
    if (!part && !(share && me.party[i].hp)) continue;
    OwnedMon &m = ownedFor((uint8_t)i);
    uint8_t foeLv = tourBattle() ? tourExpFoeLevel(TOUR, m.level, foe.level) : foe.level;
    uint32_t xp = battleExp(foe.species, foeLv, bBonus);
    xp = part ? xp / (n ? n : 1) : xp / 2;  // Repartir Exp.: la mitad a los que no pelearon
    if (!xp) xp = 1;
    uint16_t oldMax = ownedMaxHp(m);
    BattleMon before;
    monInit(before, m.species, m.level, m.genes, m.statExp);
    if (part) addStatExp(m, foe.species);
    LevelUp up = gainExp(m, xp);
    jobj(); js("k", "exp"); ji("i", i); js("n", dexName(m.species)); ji("v", xp); jend('}');
    if (!up.levels) continue;
    BattleMon after;
    monInit(after, m.species, m.level, m.genes, m.statExp);
    jobj(); js("k", "lvl"); ji("i", i); js("n", dexName(m.species)); ji("lv", m.level);
    // stats nuevas y lo que subio cada una (ventana de subida de nivel)
    const uint16_t nw[5] = { after.maxHp, after.atk, after.def, after.spe, after.spc };
    const uint16_t od[5] = { before.maxHp, before.atk, before.def, before.spe, before.spc };
    jarr("st"); for (int k = 0; k < 5; k++) jvi(nw[k]); jend(']');
    jarr("dl"); for (int k = 0; k < 5; k++) jvi(nw[k] - od[k]); jend(']');
    jend('}');
    uint8_t inBox, idx;
    ownedLoc((uint8_t)i, inBox, idx);
    for (int k = 0; k < up.newMoveCount; k++) pushPend(PD_LEARN, inBox, idx, up.newMoves[k]);
    if (up.evolveTo) pushPend(PD_EVOLVE, inBox, idx, up.evolveTo);
    // en combate suben las stats (fuera del torneo, donde el nivel es el real)
    BattleMon &b = me.party[i];
    if (!tourBattle() && !(me.active == i && me.hasSaved)) {
      BattleMon fresh;
      monInit(fresh, m.species, m.level, m.genes, m.statExp);
      uint16_t newMax = fresh.maxHp;
      b.level = fresh.level; b.atk = fresh.atk; b.def = fresh.def; b.spe = fresh.spe; b.spc = fresh.spc;
      b.maxHp = newMax;
      if (b.hp) b.hp = (uint16_t)(b.hp + (newMax > oldMax ? newMax - oldMax : 0));
      if (b.hp > b.maxHp) b.hp = b.maxHp;
    }
  }
}

static const char *EVN[] = {
  "move", "miss", "dmg", "crit", "eff", "status", "stat", "heal", "faint", "switch", "fail",
  "flinch", "confhit", "fullpara", "asleep", "frozen", "thaw", "wake", "recharge", "charge", "disobey", "item",
};

// pasa los eventos del motor a JSON con nombres y PS para animar la barra
static void jevents(uint8_t from, uint8_t act0, uint8_t act1, uint16_t *hp0, uint16_t *hp1) {
  uint8_t act[2] = { act0, act1 };
  for (int e = from; e < BT.evCount; e++) {
    const BattleEvent &ev = BT.ev[e];
    int s = ev.side;
    BattleSide &sd = BT.side[s];
    uint16_t *hp = s == 0 ? hp0 : hp1;
    if (ev.kind == EV_SWITCH) act[s] = ev.a;
    const BattleMon &m = sd.party[act[s]];
    jobj();
    js("k", ev.kind < sizeof(EVN) / sizeof(EVN[0]) ? EVN[ev.kind] : "?");
    ji("s", s);
    js("n", dexName(m.species));
    ji("a", ev.a);
    ji("b", ev.b);
    if (ev.kind == EV_MOVE || ev.kind == EV_CHARGE) {
      js("m", moveName(ev.a));
      ji("t", MOVE_TBL[ev.a].type);
      ji("p", MOVE_TBL[ev.a].power);      // para la animacion: golpe o movimiento de estado
      ji("c", MOVE_TBL[ev.a].category);
    }
    if (ev.kind == EV_EFFECT && ev.a >= 20 && ev.a < 0xF0) js("m", moveName(ev.a));
    if (ev.kind == EV_EFFECT && ev.a == AIL_DISABLE) js("m", moveName((uint8_t)ev.b));
    if (ev.kind == EV_DAMAGE && ev.a == 0) {
      int v = hp[act[s]] - ev.b;
      hp[act[s]] = (uint16_t)(v < 0 ? 0 : v);
    }
    if (ev.kind == EV_HEAL) {
      int v = hp[act[s]] + ev.b;
      hp[act[s]] = (uint16_t)(v > m.maxHp ? m.maxHp : v);
    }
    if (ev.kind == EV_DAMAGE || ev.kind == EV_HEAL || ev.kind == EV_SWITCH) {
      ji("hp", hp[act[s]]);
      ji("max", m.maxHp);
    }
    if (ev.kind == EV_SWITCH) {
      jkey("mon"); jfirst[jdepth] = true; jbattlemon(m, s == 0);
      if (s == 0) { bPartMask |= (uint8_t)(1 << ev.a); bFought |= (uint8_t)(1 << ev.a); }
      else bPartMask = (uint8_t)(1 << BT.side[0].active);
    }
    jend('}');
    if (ev.kind == EV_FAINT && s == 1) awardExp(act[1]);
    if (ev.kind == EV_FAINT && s == 0) bFaintedAny = true;
  }
}

static void snapHp(int s, uint16_t *hp) { for (int i = 0; i < BT.side[s].count; i++) hp[i] = BT.side[s].party[i].hp; }

// el rival mete al siguiente cuando se le debilita uno (los eventos van al final)
static void foeAutoSwitch(uint16_t *hp0, uint16_t *hp1) {
  if (BT.outcome || !battleNeedsSwitch(BT, 1)) return;
  uint8_t from = BT.evCount;
  uint8_t a0 = BT.side[0].active, a1 = BT.side[1].active;
  battleSwitchIn(BT, 1, battleAiPickSwitch(BT, 1));
  jevents(from, a0, a1, hp0, hp1);
}

// usa un objeto de la mochila sobre el Pokemon `idx` del combate
static int battleItem(uint8_t item, uint8_t idx) {
  if (item >= IT_COUNT || ITEMS[item].pocket != PK_MEDICINE || !TR.bag[item]) return 0;
  if (idx >= BT.side[0].count) return 0;
  if (tourBattle() && bItemsUsed >= tourItemsAllowed(TOUR)) return 0;
  BattleMon &b = BT.side[0].party[idx];
  OwnedMon tmp = ownedFor(idx);
  tmp.level = b.level;  // en copas de nivel fijo el maximo es el del combate
  tmp.hp = b.hp;
  tmp.status = b.status == STS_TOX ? (uint8_t)STS_PSN : b.status;
  UseResult r = useItemOn(TR, item, tmp);
  if (r != USE_OK) return 0;
  b.hp = tmp.hp > b.maxHp ? b.maxHp : tmp.hp;
  if (b.status != STS_NONE && tmp.status == STS_NONE) { b.status = STS_NONE; b.sleepTurns = 0; b.toxicN = 0; }
  bItemsUsed++;
  return 1;
}

// kind 0 ataque (a = hueco 0..3), 1 cambio (a = Pokemon), 2 objeto (a = objeto,
// b = Pokemon), 3 huir, 4 lanzar bola (a = bola, b = throwX10; c = acerto)
API const char *pt_battle_act(int kind, int a, int b, int c) {
  jreset();
  jobj();
  if (bKind == BK_NONE || BT.outcome || bCaught) { js("error", "over"); jend('}'); return jdone(); }
  if (battleNeedsSwitch(BT, 0) && kind != 1) { js("error", "switch"); jend('}'); return jdone(); }
  uint16_t hp0[PARTY_MAX], hp1[PARTY_MAX];
  snapHp(0, hp0);
  snapHp(1, hp1);
  uint8_t a0 = BT.side[0].active, a1 = BT.side[1].active;
  Action me;
  jarr("ev");
  if (kind == 0) {
    bool any = false;
    for (int k = 0; k < 4; k++) any |= canUseMove(BT, 0, (uint8_t)k);
    me = { ACT_MOVE, (uint8_t)(any ? a : 4) };  // 4 = Forcejeo
    if (any && !canUseMove(BT, 0, (uint8_t)a)) { jend(']'); js("error", "move"); jend('}'); return jdone(); }
  } else if (kind == 1) {
    if (a >= BT.side[0].count || !BT.side[0].party[a].hp || a == BT.side[0].active) {
      jend(']'); js("error", "switch"); jend('}'); return jdone();
    }
    if (battleNeedsSwitch(BT, 0)) {
      // cambio forzado tras un debilitamiento: no gasta turno
      uint8_t from = BT.evCount;
      battleSwitchIn(BT, 0, (uint8_t)a);
      jevents(from, a0, a1, hp0, hp1);
      jend(']');
      jkey("view"); jfirst[jdepth] = true; jview();
      jend('}');
      return jdone();
    }
    me = { ACT_SWITCH, (uint8_t)a };
  } else if (kind == 2) {
    if (!battleItem((uint8_t)a, (uint8_t)b)) { jend(']'); js("error", "item"); jend('}'); return jdone(); }
    jobj(); js("k", "useitem"); js("item", itemName((uint8_t)a)); ji("i", b);
    ji("hp", BT.side[0].party[b].hp); ji("max", BT.side[0].party[b].maxHp); jend('}');
    snapHp(0, hp0);
    BT.side[0].potions = 0;
    me = { ACT_ITEM, 0 };  // turno gastado
  } else if (kind == 3) {
    if (!BT.wild) { jend(']'); js("error", "run"); jend('}'); return jdone(); }
    me = { ACT_RUN, 0 };
  } else if (kind == 4) {
    if (!canThrow() || !ballOk(a)) { jend(']'); js("error", "ball"); jend('}'); return jdone(); }
    spillBox();
    if (TR.partyCount >= PARTY_MAX && boxFull()) { jend(']'); js("error", "boxfull"); jend('}'); return jdone(); }
    TR.bag[a]--;
    uint8_t shakes = 0;
    bool ok = c && catchAttempt(RNG, activeMon(BT, 1), (uint8_t)a, (uint8_t)b, &shakes);
    jobj(); js("k", "ball"); jb("hit", c != 0); jb("caught", ok); ji("shakes", shakes); jend('}');
    if (ok) {
      bCaught = true;
      jend(']');
      jkey("view"); jfirst[jdepth] = true; jview();
      jend('}');
      return jdone();
    }
    BT.side[0].potions = 0;
    me = { ACT_ITEM, 0 };  // en combate no huye, pero el lanzamiento gasta el turno
  } else {
    jend(']'); js("error", "kind"); jend('}'); return jdone();
  }
  Action foe = battleAiChoose(BT, 1);
  battleTurn(BT, me, foe);
  jevents(0, a0, a1, hp0, hp1);
  foeAutoSwitch(hp0, hp1);
  jend(']');
  jkey("view"); jfirst[jdepth] = true; jview();
  jend('}');
  return jdone();
}

// al terminar: copia PS/PP al equipo, premios, medallas, apagon
API const char *pt_battle_end() {
  jreset();
  jobj();
  if (bKind == BK_NONE) { js("error", "none"); jend('}'); return jdone(); }
  battleEnd(BT);
  BattleSide &me = BT.side[0];
  bool won = BT.outcome == OUT_WIN0;
  bool lost = BT.outcome == OUT_WIN1;
  bool noneFainted = !bFaintedAny;
  for (int i = 0; i < me.count; i++) if (!me.party[i].hp) noneFainted = false;
  ji("outcome", bCaught ? 9 : BT.outcome);
  if (!tourBattle()) {
    for (int i = 0; i < me.count; i++) {
      fromBattle(TR.party[i], me.party[i]);
      if (bFought >> i & 1) trustAfterBattle(TR.party[i]);
    }
  }
  uint32_t money = 0;
  if (bCaught) {
    bool ok = addCaught(TR, activeMon(BT, 1), RNG, ENC.shiny);
    if (bKind == BK_LEGEND) exploreLegendResult(EX, ENC.legend, true, NOW);
    js("caught", ok ? dexName(activeMon(BT, 1).species) : "");
    encActive = false;
  } else if (bKind == BK_WILD || bKind == BK_LEGEND) {
    if (bKind == BK_LEGEND) exploreLegendResult(EX, ENC.legend, false, NOW);
    encActive = false;
  } else if (bKind == BK_TRAINER || bKind == BK_RIVAL) {
    if (won) money = cleanWinBonus(bPrize, noneFainted, partnerInParty(TR));
    if (bKind == BK_TRAINER) encActive = false;
  } else if (bKind == BK_GYM && won) {
    GymResult g = gymVictory(TR, bGym, bAce, bLast, noneFainted);
    jb("badge", g.newBadge);
    jarr("tms");
    for (int k = 0; k < 2; k++) if (g.prize.tm[k]) jvi(g.prize.tm[k]);
    jend(']');
    if (g.prize.item) { js("prizeItem", itemName(g.prize.item)); ji("prizeN", g.prize.itemCount); }
    ji("money", g.money);
  } else if (bKind == BK_ELITE) {
    if (won) {
      eliteVictory(TR, PG, XT.leagueIdx);
      money = prizeMoney(99, BT.side[1].party[BT.side[1].count - 1].level);
      ji("beaten", XT.leagueIdx);
      if (XT.leagueIdx == ELITE_CHAMPION) { XT.leagueActive = false; jb("champion", true); }
      else XT.leagueIdx++;
    } else {
      XT.leagueActive = false;  // perder cualquier combate reinicia la corrida
      XT.leagueIdx = 0;
    }
  } else if (bKind == BK_TOUR) {
    int hp = 0, max = 0;
    for (int i = 0; i < me.count; i++) { hp += me.party[i].hp; max += me.party[i].maxHp; }
    tourReport(TOUR, won, bFaintedAny || !noneFainted, (uint8_t)(max ? hp * 100 / max : 0), BT.turn);
    jb("tourOver", tourOver(TOUR));
    ji("wins", TOUR.wins);
  }
  if (money) { earn(TR, money); ji("money", money); }
  if (lost && bKind != BK_TOUR) {
    // fuera de combate: al Centro Pokemon; con salvajes y entrenadores se pierde el 10 %
    if (bKind == BK_WILD || bKind == BK_LEGEND || bKind == BK_TRAINER || bKind == BK_RIVAL) {
      uint32_t before = TR.money;
      blackout(TR);
      ji("lostMoney", before - TR.money);
    }
    healAll(TR);
    jb("blackout", true);
  }
  ji("pending", nPend);
  jend('}');
  bKind = BK_NONE;
  return jdone();
}

// ------------------------------------------------------ aprender / evolucionar

API const char *pt_pending() {
  jreset();
  // los movimientos con hueco libre se aprenden solos
  while (nPend && PQ[0].kind == PD_LEARN) {
    OwnedMon &m = monAt(PQ[0].inBox, PQ[0].idx);
    bool known = false, free = false;
    for (int k = 0; k < 4; k++) { known |= m.moves[k] == PQ[0].a; free |= !m.moves[k]; }
    if (!known && !free) break;
    jobj();
    js("kind", known ? "known" : "learned");
    js("name", dexName(m.species));
    js("move", moveName(PQ[0].a));
    jend('}');
    if (!known) learnMove(m, PQ[0].a);
    memmove(PQ, PQ + 1, (nPend - 1) * sizeof(Pend));
    nPend--;
    return jdone();
  }
  if (!nPend) { jraw("{\"kind\":\"none\"}"); return jdone(); }
  const Pend &p = PQ[0];
  OwnedMon &m = monAt(p.inBox, p.idx);
  jobj();
  js("kind", p.kind == PD_LEARN ? "learn" : "evolve");
  js("name", dexName(m.species));
  ji("sp", m.species);
  if (p.kind == PD_LEARN) {
    js("move", moveName(p.a));
    ji("moveType", MOVE_TBL[p.a].type);
    jmoves(m.moves, m.pp);
  } else {
    ji("to", p.a);
    js("toName", dexName(p.a));
    ji("hd", BSPECIES[m.species].heightDm);  // para dibujar las dos formas a su tamano
    ji("toHd", BSPECIES[p.a].heightDm);
  }
  jend('}');
  return jdone();
}

// aprender: choice 0..3 = hueco que se olvida, 4 = no aprender. evolucionar: 1 si, 0 no
API int pt_pending_resolve(int choice) {
  if (!nPend) return 0;
  Pend p = PQ[0];
  memmove(PQ, PQ + 1, (nPend - 1) * sizeof(Pend));
  nPend--;
  OwnedMon &m = monAt(p.inBox, p.idx);
  if (p.kind == PD_LEARN) {
    if (choice >= 0 && choice < 4) return learnMove(m, p.a, (uint8_t)choice);
    return 0;
  }
  if (choice == 1 && m.species && levelEvolution(m) == p.a) {
    evolve(m, p.a);
    dexMark(TR.seen, p.a);
    dexMark(TR.caught, p.a);
    return 1;
  }
  return 0;
}

// ------------------------------------------------------- equipo y mochila ---

API int pt_party_swap(int a, int b) {
  if ((tourActive && !tourOver(TOUR)) || bKind != BK_NONE || a < 0 || b < 0 || a >= TR.partyCount || b >= TR.partyCount) return 0;
  OwnedMon x = TR.party[a]; TR.party[a] = TR.party[b]; TR.party[b] = x;
  return 1;
}
static bool teamLocked() { return bKind != BK_NONE || (tourActive && !tourOver(TOUR)); }
API int pt_team_locked() { return teamLocked(); }
API int pt_deposit(int i) { spillBox(); return !teamLocked() && depositToBox(TR, (uint8_t)i); }
API int pt_withdraw(int i) { return !teamLocked() && withdrawFromBox(TR, (uint8_t)i); }

// liberar un Pokemon del PC o del equipo (nunca al companero ni al ultimo del equipo)
API int pt_release(int inBox, int idx) {
  if (teamLocked()) return 0;
  if (inBox) {
    if (idx < 0 || idx >= TR.boxCount) return 0;
    for (int i = idx; i + 1 < TR.boxCount; i++) TR.box[i] = TR.box[i + 1];
    TR.boxCount--;
    return 1;
  }
  if (idx < 0 || idx >= TR.partyCount || TR.partyCount <= 1 || (TR.party[idx].flags & OM_PARTNER)) return 0;
  for (int i = idx; i + 1 < TR.partyCount; i++) TR.party[i] = TR.party[i + 1];
  TR.partyCount--;
  return 1;
}

// objeto fuera de combate; devuelve UseResult (USE_OK = 0) o 9 si no se puede
API const char *pt_use_item(int item, int idx) {
  jreset();
  jobj();
  if (bKind != BK_NONE || idx < 0 || idx >= TR.partyCount || item < 0 || item >= IT_COUNT) { js("res", "error"); jend('}'); return jdone(); }
  OwnedMon &m = TR.party[idx];
  if (item == IT_LINK_CABLE) {
    // Cable Link (Battle Shop): evoluciona a uno que evoluciona por intercambio
    uint8_t to = TR.bag[item] ? tradeEvolution(m.species) : 0;
    if (!to) { js("res", "noeffect"); jend('}'); return jdone(); }
    evolve(m, to);
    TR.bag[item]--;
    dexMark(TR.seen, to);
    dexMark(TR.caught, to);
    js("res", "evolved"); js("to", dexName(to));
    jend('}');
    return jdone();
  }
  uint8_t before = m.species;
  LevelUp up;
  UseResult r = useItemOn(TR, (uint8_t)item, m, &up);
  static const char *RES[] = { "ok", "noitem", "noeffect", "evolved", "levelup" };
  js("res", RES[r]);
  if (r == USE_EVOLVED) js("to", dexName(m.species));
  if (r == USE_LEVEL_UP) {
    ji("lv", m.level);
    for (int k = 0; k < up.newMoveCount; k++) pushPend(PD_LEARN, 0, (uint8_t)idx, up.newMoves[k]);
    if (up.evolveTo) pushPend(PD_EVOLVE, 0, (uint8_t)idx, up.evolveTo);
  }
  (void)before;
  jend('}');
  return jdone();
}

// MT/MO numero `num` (1..55) en el Pokemon idx, olvidando `slot` si no hay hueco
API int pt_teach(int num, int idx, int slot) {
  if (num < 1 || num > NUM_MACHINES || idx < 0 || idx >= TR.partyCount) return 0;
  uint8_t mi = (uint8_t)(num - 1);
  if (!TR.tms[mi]) return 0;
  OwnedMon &m = TR.party[idx];
  if (!canLearnMachine(m.species, mi)) return 0;
  uint8_t move = MACHINE_MOVE[mi];
  for (int k = 0; k < 4; k++) if (m.moves[k] == move) return 0;
  if (!learnMove(m, move, slot < 0 ? 0xFF : (uint8_t)slot)) return 0;
  if (num <= NUM_TMS) TR.tms[mi]--;  // las MT de gen 1 se gastan; las MO no
  return 1;
}

API int pt_can_learn_tm(int num, int idx) {
  if (num < 1 || num > NUM_MACHINES || idx < 0 || idx >= TR.partyCount) return 0;
  return canLearnMachine(TR.party[idx].species, (uint8_t)(num - 1));
}

API int pt_heal(int atHome) {
  if (bKind != BK_NONE) return 0;
  if (atHome) { healAll(TR); XT.healAt = 0; return 1; }
  if (!XT.healAt) XT.healAt = NOW + 1800;  // fuera de casa: 30 minutos
  return 2;
}

API int pt_hatch(int i) {
  OwnedMon m;
  spillBox();
  if (!eggHatch(EX, TR, RNG, (uint8_t)i, &m)) return 0;
  return m.species;
}

// --------------------------------------------------------------- tiendas ----

static void jshopItem(uint8_t item, uint32_t price) {
  jobj(); ji("id", item); js("name", itemName(item)); ji("price", price); ji("have", TR.bag[item]); jend('}');
}

API const char *pt_shop(int which) {
  jreset();
  jobj();
  ji("money", TR.money);
  ji("bp", TR.bp);
  jarr("items");
  if (which == 0) {
    for (int i = 0; i < IT_COUNT; i++) if (martSells(TR, (uint8_t)i)) jshopItem((uint8_t)i, ITEMS[i].price);
  } else if (which == 1) {
    if (celadonOpen(TR))
      for (int i = 0; i < IT_COUNT; i++) if (celadonSells((uint8_t)i)) jshopItem((uint8_t)i, ITEMS[i].price);
  } else if (which == 2) {
    for (int i = 0; i < IT_COUNT; i++) if (TR.bag[i] && sellPrice((uint8_t)i)) jshopItem((uint8_t)i, sellPrice((uint8_t)i));
  } else if (which == 3) {
    for (int i = 0; i < NUM_BATTLE_SHOP; i++) {
      const BattleShopOffer &o = BATTLE_SHOP[i];
      jobj();
      ji("idx", i);
      jb("tm", o.kind == BS_TM);
      ji("id", o.id);
      if (o.kind == BS_TM) { char b[16]; snprintf(b, sizeof(b), "TM%02d", o.id); js("label", b); js("name", moveName(MACHINE_MOVE[o.id - 1])); }
      else js("name", itemName(o.id));
      ji("bp", o.bp);
      jend('}');
    }
  }
  jend(']');
  if (which == 1) {
    jb("open", celadonOpen(TR));
    uint8_t tms[CELADON_TMS];
    celadonTms(weekIndex(NOW), tms);
    jarr("tms");
    for (int k = 0; k < CELADON_TMS; k++) {
      jobj(); ji("slot", k); ji("num", tms[k]); js("name", moveName(MACHINE_MOVE[tms[k] - 1])); ji("price", tmPrice(tms[k])); jend('}');
    }
    jend(']');
  }
  jend('}');
  return jdone();
}

// which: 0 Poke Mart, 1 Celadon, 2 vender, 3 Battle Shop (id = indice), 4 MT de Celadon (id = hueco)
API int pt_buy(int which, int id, int qty) {
  switch (which) {
    case 0: return martBuy(TR, (uint8_t)id, (uint8_t)qty);
    case 1: return celadonBuy(TR, (uint8_t)id, (uint8_t)qty);
    case 2: return sellItem(TR, (uint8_t)id, (uint8_t)qty);
    case 3: return battleShopBuy(TR, PG, (uint8_t)id, NOW);
    case 4: return celadonBuyTm(TR, weekIndex(NOW), (uint8_t)id);
  }
  return SHOP_NOT_SOLD;
}

// --------------------------------------------------------------- gimnasios -

API const char *pt_gyms() {
  jreset();
  jobj();
  ji("km", EX.meters / 1000);
  jarr("gyms");
  for (int g = 0; g < NUM_GYMS; g++) {
    jobj();
    js("leader", GYMS[g].leader);
    ji("type", GYMS[g].type);
    ji("km", GYMS[g].kmToUnlock);
    jb("open", gymOpen((uint8_t)g));
    jb("badge", TR.badges & (1 << g));
    ji("size", GYMS[g].teamSize);
    jend('}');
  }
  jend(']');
  jb("leagueOpen", leagueOpen());
  jb("leagueActive", XT.leagueActive);
  ji("leagueIdx", XT.leagueIdx);
  jarr("elite");
  for (int i = 0; i < NUM_ELITE; i++) { jobj(); js("name", ELITE[i].name); ji("type", ELITE[i].type); jb("beaten", PG.eliteBeaten & (1 << i)); jend('}'); }
  jend(']');
  jend('}');
  return jdone();
}

API void pt_league_quit() { XT.leagueActive = false; XT.leagueIdx = 0; }

// ---------------------------------------------------------------- torneos --

API const char *pt_tour_info() {
  jreset();
  jobj();
  Featured f = featuredCup(weekIndex(NOW));
  ji("featured", f.cup);
  ji("featuredType", f.type);
  jarr("cups");
  for (int c = 0; c < NUM_CUPS; c++) { jobj(); js("name", cupName((uint8_t)c)); js("city", CUPS[c].city); jend('}'); }
  jend(']');
  jarr("tiers");
  for (int t = 0; t < NUM_TOUR_TIERS; t++) {
    jobj();
    js("name", gLang == LANG_ES ? TOUR_TIERS[t].es : TOUR_TIERS[t].en);
    ji("fee", TOUR_TIERS[t].fee);
    ji("champBp", TOUR_TIERS[t].champBp);
    ji("finBp", TOUR_TIERS[t].finBp);
    jend('}');
  }
  jend(']');
  DailyChallenge dc = dailyChallenge(dayIndex(NOW));
  jobj("daily");
  ji("rule", dc.rule);
  ji("type", dc.type);
  ji("prizeKind", dc.prizeKind);
  if (dc.prizeKind == PZ_ITEM) js("prize", itemName(dc.prizeId));
  else if (dc.prizeKind == PZ_TM) { char b[48]; snprintf(b, sizeof(b), "TM%02d", dc.prizeId); js("prize", b); }
  jb("played", dailyPlayedToday(PG, NOW));
  ji("days", PG.dcDays);
  ji("score", PG.dcScore);
  jend('}');
  jb("active", tourActive);
  if (tourActive) {
    ji("cup", TOUR.cup);
    ji("tier", TOUR.tier);
    jb("isDaily", TOUR.daily);
    ji("wins", TOUR.wins);
    jb("over", tourOver(TOUR));
    ji("oppLevel", TOUR.oppLevel);
  }
  jend('}');
  return jdone();
}

static void decodePicks(int p0, int p1, int p2, TourPick *out) {
  int v[3] = { p0, p1, p2 };
  for (int i = 0; i < TOUR_TEAM; i++) out[i] = { (uint8_t)(v[i] >> 7 & 1), (uint8_t)(v[i] & 0x7F) };
}

// picks: (enPC << 7) | indice. Devuelve TourResult (0 = OK)
API int pt_tour_enter(int cup, int tier, int p0, int p1, int p2) {
  if (tourActive && !tourOver(TOUR)) return TOUR_LOCKED;
  TourPick picks[TOUR_TEAM];
  decodePicks(p0, p1, p2, picks);
  int r = tourEnter(TOUR, TR, PG, (uint8_t)cup, (uint8_t)tier, picks, NOW, RNG);
  tourActive = r == TOUR_OK;
  return r;
}

API int pt_daily_enter(int p0, int p1, int p2) {
  if (tourActive && !tourOver(TOUR)) return TOUR_LOCKED;
  TourPick picks[TOUR_TEAM];
  decodePicks(p0, p1, p2, picks);
  int r = dailyEnter(TOUR, TR, PG, EX, picks, NOW, RNG);
  tourActive = r == TOUR_OK;
  return r;
}

API const char *pt_tour_finish() {
  jreset();
  jobj();
  if (!tourActive) { js("error", "none"); jend('}'); return jdone(); }
  TourPayout p = tourFinish(TOUR, TR, PG, EX, NOW);
  tourActive = false;
  ji("money", p.money);
  ji("bp", p.bp);
  jb("champion", p.champion);
  jb("finalist", p.finalist);
  jb("perfect", p.perfect);
  jb("special", p.specialBeaten);
  if (p.prizeKind == PZ_ITEM) js("prize", itemName(p.prizeId));
  else if (p.prizeKind == PZ_TM) { char b[16]; snprintf(b, sizeof(b), "TM%02d", p.prizeId); js("prize", b); }
  if (p.weeklyKind == PZ_ITEM) js("weekly", itemName(p.weeklyId));
  else if (p.weeklyKind == PZ_TM) { char b[16]; snprintf(b, sizeof(b), "TM%02d", p.weeklyId); js("weekly", b); }
  else if (p.weeklyKind == PZ_RIBBON) js("weekly", "ribbon");
  ji("pending", nPend);
  jend('}');
  return jdone();
}

// --------------------------------------------------------------- utilidades -

API const char *pt_type_name(int t) { jreset(); jtext(t == T_NONE ? "" : typeName((uint8_t)t)); return jdone(); }
API int pt_type_eff(int moveType, int t1, int t2) {
  int e = TYPE_EFF[moveType][t1];
  if (t2 != T_NONE) e = e * TYPE_EFF[moveType][t2] / 2;
  return e;  // 2 = normal
}
