#!/usr/bin/env python3
"""Genera battle_data.h (tablas de batalla de gen 1 para el firmware) desde
tools/battle_data.py, que a su vez sale de tools/fetch_battle_data.py.

  python3 tools/gen_battle.py
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from battle_data import (TYPES, TYPE_NAMES_ES, TYPE_CHART, SPECIES, MOVES, MACHINES,
                         LEARNSET, TM_COMPAT, EVOLUTIONS)

GROWTHS = ['fast', 'medium', 'medium-slow', 'slow']
STATS = ['attack', 'defense', 'speed', 'special', 'accuracy', 'evasion']
STONES = ['fire-stone', 'water-stone', 'thunder-stone', 'leaf-stone', 'moon-stone']
# objetivos que apuntan al propio usuario (para saber a quien tocan stat_changes)
SELF_TARGETS = ('user', 'users-field', 'user-and-allies', 'user-or-ally')

# Bugs de gen 1 que PokeTrek corrige (ver reglas, seccion Battles)
CHART_FIXES = {('ghost', 'psychic'): 2.0}  # en gen 1 era 0 por error


def ident(slug):
    return slug.upper().replace('-', '_')


# La fuente 5x7 del firmware es CP437 y la UI centra con strlen()*6, asi que
# los acentos van como un byte octal (igual que en i18n.cpp), nunca en UTF-8.
# Las mayusculas acentuadas que CP437 no tiene pierden el acento.
CP437 = {'á': '\\240', 'é': '\\202', 'í': '\\241', 'ó': '\\242', 'ú': '\\243',
         'ñ': '\\244', 'Ñ': '\\245', 'ü': '\\201', 'Ü': '\\232', 'É': '\\220',
         'Á': 'A', 'Í': 'I', 'Ó': 'O', 'Ú': 'U'}


def cstr(s):
    out = []
    for ch in s.replace('\\', '\\\\').replace('"', '\\"'):
        if ord(ch) < 128:
            out.append(ch)
        elif ch in CP437:
            out.append(CP437[ch])
        else:
            raise SystemExit(f'caracter sin equivalente CP437 en {s!r}: {ch!r}')
    # un octal seguido de digito se leeria como parte del escape: se corta la cadena
    return '"' + '""'.join(_split_octal(''.join(out))) + '"'


def _split_octal(s):
    parts, i = [''], 0
    while i < len(s):
        if s[i] == '\\' and i + 3 < len(s) + 1 and s[i + 1:i + 4].isdigit():
            parts[-1] += s[i:i + 4]
            i += 4
            if i < len(s) and s[i].isdigit():
                parts.append('')
            continue
        parts[-1] += s[i]
        i += 1
    return parts


def flee_pct(capture_rate):
    # PoGo API no publica tasas de huida, asi que salen del capture rate de gen 1:
    # 255 (Pidgey) -> 5 %, 3 (Mewtwo) -> 25 %
    return 5 + (255 - capture_rate) * 20 // 252


def effect_chance(m):
    # effect_chance ya viene corregido a gen 1; si falta, el de meta
    if m['effect_chance'] is not None:
        return m['effect_chance']
    return max(m['ailment_chance'], m['stat_chance'], m['flinch_chance'])


def flinch_pct(m):
    # el % de retroceso de gen 1 es el effect_chance corregido (Mordisco 10 %, no 30)
    if m['flinch_chance'] and m['effect_chance'] is not None:
        return m['effect_chance']
    return m['flinch_chance']


def dedupe(learn):
    # PokeAPI repite alguno (Mewtwo: Psiquico a nivel 1 y 66): vale el primero
    seen, out = set(), []
    for lv, mv in learn:
        if mv not in seen:
            seen.add(mv)
            out.append((lv, mv))
    return out


def main():
    ailments = sorted({m['ailment'] for m in MOVES.values()} - {'none'})
    categories = sorted({m['category'] for m in MOVES.values()})
    machines = list(MACHINES)

    out = []
    out.append("#pragma once\n#include <stdint.h>\n#include \"i18n.h\"  // gLang\n\n")
    out.append("// GENERADO por tools/gen_battle.py desde tools/battle_data.py - no editar\n\n")

    out.append("// ---- tipos (15, orden de gen 1) ----\n")
    out.append("enum BType : uint8_t {\n")
    for t in TYPES:
        out.append(f"  T_{ident(t)},\n")
    out.append("  T_COUNT,\n  T_NONE = 0xFF  // segundo tipo vacio\n};\n\n")
    out.append("static const char *const TYPE_NAME_EN[T_COUNT] = {\n  "
               + ", ".join(cstr(t.capitalize()) for t in TYPES) + "\n};\n")
    out.append("static const char *const TYPE_NAME_ES[T_COUNT] = {\n  "
               + ", ".join(cstr(TYPE_NAMES_ES[t]) for t in TYPES) + "\n};\n\n")

    out.append("// eficacia x2: 0 inmune, 1 = x0.5, 2 = x1, 4 = x2 [atacante][defensor].\n")
    out.append("// Gen 1 tal cual salvo Fantasma > Psiquico, que aqui es x2 (bug corregido).\n")
    out.append("static const uint8_t TYPE_EFF[T_COUNT][T_COUNT] = {\n")
    for a in TYPES:
        row = []
        for d in TYPES:
            mult = CHART_FIXES.get((a, d), TYPE_CHART.get(f'{a}>{d}', 1.0))
            row.append(str(int(mult * 2)))
        out.append(f"  {{ {', '.join(row)} }},  // {a}\n")
    out.append("};\n\n")

    out.append("enum Growth : uint8_t { " + ", ".join(f"G_{ident(g)}" for g in GROWTHS) + " };\n")
    out.append("enum BStat : uint8_t { ST_NONE = 0, " + ", ".join(f"ST_{ident(s)}" for s in STATS) + " };\n")
    out.append("enum Ailment : uint8_t { AIL_NONE = 0, " + ", ".join(f"AIL_{ident(a)}" for a in ailments) + " };\n")
    out.append("enum MoveCat : uint8_t { " + ", ".join(f"MC_{ident(c)}" for c in categories) + " };\n")
    out.append("enum Stone : uint8_t { " + ", ".join(f"STONE_{ident(s.replace('-stone', ''))}" for s in STONES) + " };\n")
    out.append("enum EvoMethod : uint8_t { EVO_LEVEL, EVO_STONE, EVO_TRADE };\n\n")

    out.append("// ---- movimientos: indice = id de PokeAPI (1..165), 0 sin usar ----\n")
    out.append(
        "struct MoveInfo {\n"
        "  const char *en, *es;\n"
        "  uint8_t type;\n"
        "  uint8_t power;      // 0 = sin poder fijo (estado o dano especial)\n"
        "  uint8_t accuracy;   // 0 = nunca falla\n"
        "  uint8_t pp;\n"
        "  int8_t priority;\n"
        "  uint8_t category;   // MoveCat\n"
        "  uint8_t ailment;    // Ailment que puede causar\n"
        "  uint8_t chance;     // % del efecto secundario; 0 = siempre\n"
        "  uint8_t flinch;     // % de retroceso\n"
        "  uint8_t highCrit;   // 1 = critico alto (Cuchillada, Hoja Afilada...)\n"
        "  int8_t drain;       // % del dano que recupera (<0 = retroceso)\n"
        "  int8_t healing;     // % de PS que cura\n"
        "  uint8_t minHits, maxHits, minTurns, maxTurns;\n"
        "  uint8_t stat1; int8_t delta1;\n"
        "  uint8_t stat2; int8_t delta2;\n"
        "  uint8_t selfStat;   // 1 = los cambios de stat son para el usuario\n"
        "};\n\n")
    out.append(f"#define NUM_MOVES {len(MOVES)}\n")
    out.append("static const MoveInfo MOVE_TBL[NUM_MOVES + 1] = {\n")
    out.append('  { "-", "-", T_NORMAL, 0, 0, 0, 0, 0, AIL_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, ST_NONE, 0, ST_NONE, 0, 0 },\n')
    for mid in sorted(MOVES):
        m = MOVES[mid]
        sc = list(m['stat_changes']) + [(None, 0), (None, 0)]
        (s1, d1), (s2, d2) = sc[0], sc[1]
        st1 = f"ST_{ident(s1)}" if s1 else 'ST_NONE'
        st2 = f"ST_{ident(s2)}" if s2 else 'ST_NONE'
        ail = f"AIL_{ident(m['ailment'])}" if m['ailment'] != 'none' else 'AIL_NONE'
        out.append(
            f"  {{ {cstr(m['en'])}, {cstr(m['es'])}, T_{ident(m['type'])}, {m['power'] or 0}, "
            f"{m['accuracy'] or 0}, {m['pp']}, {m['priority']}, MC_{ident(m['category'])}, {ail}, "
            f"{effect_chance(m)}, {flinch_pct(m)}, {1 if m['crit_rate'] else 0}, {m['drain']}, "
            f"{m['healing']}, {m['min_hits']}, {m['max_hits']}, {m['min_turns']}, {m['max_turns']}, "
            f"{st1}, {d1}, {st2}, {d2}, {1 if m['target'] in SELF_TARGETS else 0} }},  // {mid} {m['slug']}\n")
    out.append("};\n\n")

    out.append("// ---- MT/MO de Rojo/Azul: 0..49 = MT01..MT50, 50..54 = MO01..MO05 ----\n")
    out.append(f"#define NUM_MACHINES {len(machines)}\n")
    out.append("static const uint8_t MACHINE_MOVE[NUM_MACHINES] = {\n")
    for i in range(0, len(machines), 10):
        chunk = machines[i:i + 10]
        out.append("  " + ", ".join(f"{MACHINES[tm]:3d}" for tm in chunk)
                   + f",  // {chunk[0]}..{chunk[-1]}\n")
    out.append("};\n\n")

    out.append("// ---- aprendizaje por nivel (Rojo/Azul); nivel 1 = movimientos iniciales ----\n")
    out.append("struct LearnEntry { uint8_t level, move; };\n")
    out.append("static const LearnEntry LEARN_TBL[] = {\n")
    starts = {}
    pos = 0
    for num in sorted(LEARNSET):
        learn = dedupe(LEARNSET[num])
        starts[num] = (pos, len(learn))
        row = ", ".join(f"{{{lv}, {mv}}}" for lv, mv in learn)
        out.append(f"  {row},  // {num} {SPECIES[num]['slug']}\n")
        pos += len(learn)
    out.append("};\n\n")

    out.append("// ---- especies: indice = numero de Pokedex, 0 sin usar ----\n")
    out.append(
        "struct BattleSpecies {\n"
        "  uint8_t type1, type2;          // type2 = T_NONE si es de un tipo\n"
        "  uint8_t hp, atk, def, spe, spc;  // base stats de gen 1 (Special unico)\n"
        "  uint8_t captureRate;           // 3..255\n"
        "  uint16_t baseExp;              // la actual de PokeAPI\n"
        "  uint8_t growth;                // Growth\n"
        "  uint8_t flee;                  // % de huida tras un lanzamiento fallido\n"
        "  uint16_t learnStart; uint8_t learnCount;  // tramo en LEARN_TBL\n"
        "  uint64_t machines;             // bit i = puede aprender MACHINE_MOVE[i]\n"
        "};\n\n")
    out.append("static const BattleSpecies BSPECIES[152] = {\n")
    out.append("  { T_NORMAL, T_NONE, 0, 0, 0, 0, 0, 0, 0, G_MEDIUM, 0, 0, 0, 0ULL },\n")
    for num in sorted(SPECIES):
        s = SPECIES[num]
        t1, t2 = s['types']
        t2c = f"T_{ident(t2)}" if t2 else 'T_NONE'
        mask = 0
        for tm in TM_COMPAT[num]:
            mask |= 1 << machines.index(tm)
        st, cnt = starts[num]
        out.append(
            f"  {{ T_{ident(t1)}, {t2c}, {s['hp']}, {s['attack']}, {s['defense']}, {s['speed']}, "
            f"{s['special']}, {s['capture_rate']}, {s['base_exp']}, G_{ident(s['growth'])}, "
            f"{flee_pct(s['capture_rate'])}, {st}, {cnt}, 0x{mask:014X}ULL }},  // {num} {s['slug']}\n")
    out.append("};\n\n")

    out.append("// ---- evoluciones de gen 1 (nivel, piedra o intercambio) ----\n")
    out.append("struct EvoEntry { uint8_t from, to, method, param; };  // param: nivel o Stone\n")
    out.append(f"#define NUM_EVOS {len(EVOLUTIONS)}\n")
    out.append("static const EvoEntry EVO_TBL[NUM_EVOS] = {\n")
    for frm, to, how, param in EVOLUTIONS:
        if how == 'level':
            out.append(f"  {{ {frm}, {to}, EVO_LEVEL, {param} }},\n")
        elif how == 'item':
            out.append(f"  {{ {frm}, {to}, EVO_STONE, STONE_{ident(param.replace('-stone', ''))} }},\n")
        else:
            out.append(f"  {{ {frm}, {to}, EVO_TRADE, 0 }},\n")
    out.append("};\n\n")

    out.append(
        "static inline const char *typeName(uint8_t t) {\n"
        "  if (t >= T_COUNT) return \"\";\n"
        "  return gLang == LANG_ES ? TYPE_NAME_ES[t] : TYPE_NAME_EN[t];\n"
        "}\n\n"
        "static inline const char *moveName(uint8_t id) {\n"
        "  if (id == 0 || id > NUM_MOVES) return MOVE_TBL[0].en;\n"
        "  return gLang == LANG_ES ? MOVE_TBL[id].es : MOVE_TBL[id].en;\n"
        "}\n\n"
        "// multiplicador x2 contra un defensor de uno o dos tipos: 0, 1, 2, 4 u 8 (= x4)\n"
        "static inline uint8_t typeEff2(uint8_t atk, uint8_t def1, uint8_t def2) {\n"
        "  uint16_t m = TYPE_EFF[atk][def1];\n"
        "  if (def2 != T_NONE) m = m * TYPE_EFF[atk][def2] / 2;\n"
        "  return (uint8_t)m;\n"
        "}\n")

    path = os.path.join(os.path.dirname(__file__), '..', 'battle_data.h')
    with open(path, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write(''.join(out))
    print(f"guardado {os.path.normpath(path)} ({len(SPECIES)} especies, {len(MOVES)} movimientos, "
          f"{pos} entradas de aprendizaje, {len(EVOLUTIONS)} evoluciones)")


if __name__ == '__main__':
    main()
