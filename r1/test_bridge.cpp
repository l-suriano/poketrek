// Prueba nativa del puente del R1: juega una partida automatica llamando a
// las mismas funciones pt_* que usa la interfaz web e imprime cada JSON en una
// linea (r1/check_json.py comprueba que todos sean JSON validos).
//   make -C r1 test
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

extern "C" {
int pt_buf_size();
uint8_t *pt_buf();
void pt_init(int, uint32_t, uint32_t);
void pt_new(int);
int pt_save();
int pt_load(int);
void pt_tick(uint32_t);
const char *pt_state();
const char *pt_walk(int, int);
const char *pt_next_event();
const char *pt_routes();
const char *pt_search(int);
const char *pt_encounter();
void pt_leave_encounter();
const char *pt_throw(int, int, int);
const char *pt_battle_start(int, int);
const char *pt_battle_view();
const char *pt_battle_act(int, int, int, int);
const char *pt_battle_end();
const char *pt_pending();
int pt_pending_resolve(int);
const char *pt_use_item(int, int);
const char *pt_shop(int);
int pt_buy(int, int, int);
const char *pt_gyms();
const char *pt_challenges();
void pt_dev_badges(int);
void pt_dev_route(int, int);
const char *pt_tour_info();
int pt_tour_enter(int, int, int, int, int);
int pt_daily_enter(int, int, int);
const char *pt_tour_finish();
const char *pt_box();
const char *pt_mon(int, int);
const char *pt_dex();
int pt_heal(int);
int pt_incense_start(int);
const char *pt_incense_tick();
const char *pt_type_name(int);
}

static FILE *OUT;
static int fails = 0;
static void out(const char *tag, const char *json) { fprintf(OUT, "%s\t%s\n", tag, json); }
static void check(bool ok, const char *what) { if (!ok) { printf("FALLO: %s\n", what); fails++; } }

// combate con un movimiento al azar hasta que acabe (cambia si hace falta)
static int playBattle(uint32_t &rng) {
  for (int t = 0; t < 400; t++) {
    const char *v = pt_battle_view();
    if (strstr(v, "\"outcome\":0") == nullptr) break;
    if (strstr(v, "\"needSwitch\":true")) {
      for (int i = 0; i < 6; i++) {
        const char *r = pt_battle_act(1, i, 0, 0);
        if (!strstr(r, "\"error\"")) { out("switch", r); break; }
      }
      continue;
    }
    rng = rng * 1103515245u + 12345u;
    int slot = (rng >> 16) % 4;
    const char *r = pt_battle_act(0, slot, 0, 0);
    if (strstr(r, "\"error\":\"move\"")) r = pt_battle_act(0, 0, 0, 0);
    if (strstr(r, "\"error\":\"move\"")) r = pt_battle_act(0, 1, 0, 0);
    out("act", r);
  }
  const char *e = pt_battle_end();
  out("end", e);
  for (int k = 0; k < 30; k++) {
    const char *p = pt_pending();
    out("pending", p);
    if (strstr(p, "\"kind\":\"none\"")) break;
    if (strstr(p, "\"kind\":\"learn\"")) pt_pending_resolve(0);
    else if (strstr(p, "\"kind\":\"evolve\"")) pt_pending_resolve(1);
  }
  return 1;
}

int main(int argc, char **argv) {
  OUT = fopen(argc > 1 ? argv[1] : "bridge_out.txt", "w");
  uint32_t now = 1793577600u + 10 * 3600;  // lunes 10:00
  uint32_t rng = 7;
  pt_init(1, now, 12345);
  pt_new(4);  // Charmander
  out("state", pt_state());
  check(strstr(pt_state(), "\"money\":3000") != nullptr, "dinero inicial");

  int battles = 0, catches = 0, trainers = 0;
  for (int day = 0; day < 6; day++) {
    now += 86400;
    pt_tick(now);
    pt_heal(1);
    out("walk", pt_walk(4000, 1));
    for (int e = 0; e < 30; e++) {
      const char *ev = pt_next_event();
      out("event", ev);
      if (strstr(ev, "\"kind\":\"none\"")) break;
      if (strstr(ev, "\"kind\":\"wild\"")) {
        if (e % 2) {
          out("throw", pt_throw(0, 16, 1));
          pt_leave_encounter();
          catches++;
        } else {
          out("start", pt_battle_start(1, 0));
          playBattle(rng);
          battles++;
        }
      } else if (strstr(ev, "\"kind\":\"trainer\"")) {
        out("start", pt_battle_start(1, 0));
        playBattle(rng);
        trainers++;
      }
      pt_heal(1);
    }
    // busquedas desde casa con la EE
    out("routes", pt_routes());
    for (int s = 0; s < 4; s++) {
      const char *r = pt_search(day % 2);
      out("search", r);
      if (strstr(r, "\"kind\":\"wild\"") || strstr(r, "\"kind\":\"trainer\"")) {
        out("start", pt_battle_start(1, 0));
        playBattle(rng);
        battles++;
      }
      pt_heal(1);
    }
    out("buy", pt_shop(0));
    pt_buy(0, 0, 3);
    pt_buy(0, 4, 2);
  }
  printf("combates %d, lanzamientos %d, entrenadores %d\n", battles, catches, trainers);
  check(battles > 5, "hubo combates");

  // retos de ruta: Brock pide la Ruta 1 y el Bosque Verde (3 combates en orden)
  out("challenges", pt_challenges());
  check(strstr(pt_gyms(), "\"route\":0") != nullptr, "Brock pide el reto 0");
  check(strstr(pt_battle_start(6, 1), "\"error\"") != nullptr, "Mt. Moon cerrado sin medalla");
  for (int t = 0; t < 40 && !strstr(pt_challenges(), "\"cleared\":true"); t++) {
    pt_heal(1);
    const char *s = pt_battle_start(6, 0);
    out("routestart", s);
    if (strstr(s, "\"error\"")) break;
    playBattle(rng);
  }
  check(strstr(pt_challenges(), "\"cleared\":true") != nullptr, "reto 0 superado");
  check(strstr(pt_battle_start(6, 0), "\"error\"") != nullptr, "reto superado no se repite");
  check(strstr(pt_gyms(), "\"route\":-1") != nullptr, "Brock ya no pide reto");

  // gimnasio de Brock
  out("gyms", pt_gyms());
  pt_heal(1);
  out("gymstart", pt_battle_start(2, 0));
  playBattle(rng);

  // objeto y pokedex
  out("use", pt_use_item(4, 0));
  out("dex", "\"x\"");
  check(strlen(pt_dex()) == 151, "pokedex de 151");
  out("box", pt_box());
  out("mon", pt_mon(0, 0));
  out("type", pt_type_name(11));

  // torneo: hace falta un equipo de 3
  out("tour", pt_tour_info());
  int r = pt_tour_enter(0, 0, 0, 1, 2);
  printf("inscripcion torneo: %d\n", r);
  if (r == 0) {
    for (int round = 0; round < 3; round++) {
      const char *s = pt_battle_start(4, 0);
      out("tourstart", s);
      if (strstr(s, "\"error\"")) break;
      playBattle(rng);
      if (strstr(pt_tour_info(), "\"over\":true")) break;
    }
    out("tourend", pt_tour_finish());
  }
  r = pt_daily_enter(0, 1, 2);
  printf("daily: %d\n", r);
  if (r == 0) {
    for (int round = 0; round < 3; round++) {
      const char *s = pt_battle_start(4, 0);
      if (strstr(s, "\"error\"")) break;
      playBattle(rng);
      if (strstr(pt_tour_info(), "\"over\":true")) break;
    }
    out("dailyend", pt_tour_finish());
  }

  // jefes: Snorlax (con 4 medallas) y Silph S.A. (rival y Giovanni, con 5)
  pt_dev_badges(0x0F);
  pt_heal(1);
  const char *sn = pt_battle_start(6, 7);
  out("snorlax", sn);
  check(strstr(sn, "\"kind\":\"wild\"") && strstr(sn, "\"route\":7"), "Snorlax salvaje del reto 7");
  playBattle(rng);
  pt_dev_badges(0x1F);
  pt_heal(1);
  out("silph", pt_battle_start(6, 8));
  playBattle(rng);
  pt_dev_route(8, 3);
  pt_heal(1);
  const char *rv = pt_battle_start(6, 8);
  out("rival", rv);
  check(strstr(rv, "\"kind\":\"rival\"") && strstr(rv, "\"boss\":true"), "rival en Silph");
  playBattle(rng);
  pt_dev_route(8, 4);
  pt_heal(1);
  const char *gv = pt_battle_start(6, 8);
  out("giovanni", gv);
  check(strstr(gv, "\"tgym\":7") && strstr(gv, "\"foeName\":\"Giovanni\"") && strstr(gv, "\"count\":4"), "Giovanni en Silph con 4");
  playBattle(rng);

  // guardar y cargar
  char before[8192];
  snprintf(before, sizeof(before), "%s", pt_state());
  int n = pt_save();
  check(n > 0 && n < pt_buf_size(), "guardar");
  pt_new(1);
  check(pt_load(n) == 1, "cargar");
  check(strcmp(before, pt_state()) == 0, "estado igual tras cargar");
  check(strstr(pt_challenges(), "\"cleared\":true") != nullptr, "retos guardados");
  pt_buf()[40] ^= 0x55;
  check(pt_load(n) == 0, "guardado corrupto rechazado");
  out("final", pt_state());
  fclose(OUT);
  printf(fails ? "%d fallos\n" : "OK\n", fails);
  return fails ? 1 : 0;
}
