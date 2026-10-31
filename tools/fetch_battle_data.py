#!/usr/bin/env python3
"""Descarga de PokeAPI los datos de batalla de gen 1 y los vuelca
en tools/battle_data.py, que se versiona para que CI regenere battle_data.h sin red.

  python3 tools/fetch_battle_data.py

Las respuestas se guardan en tools/downloads/pokeapi/ (ignorado por git), asi que
volver a ejecutarlo no repite peticiones. Borra esa carpeta para refrescar.

Que se toma de gen 1 y que no:
- tipos de especie: past_types (Clefairy normal, Magnemite electrico puro)
- stats base: past_stats de todas las generaciones (Special de generation-i y
  los valores anteriores a los cambios de gen 6, en generation-v)
- movimientos: tipo/poder/precision/PP de gen 1 aplicando past_values
- tabla de tipos: past_damage_relations de generation-i (15 tipos)
- aprendizaje por nivel y MT/MO: version group red-blue
- experiencia base: la actual de PokeAPI (gen 1 no esta en la API)
- efectos secundarios (meta, stat_changes): los actuales, con las correcciones
  de MOVE_FIXES para los que cambiaron desde gen 1
"""
import json
import os
import pprint
import time
import urllib.request

API = 'https://pokeapi.co/api/v2/'
CACHE = os.path.join(os.path.dirname(__file__), 'downloads', 'pokeapi')
OUT = os.path.join(os.path.dirname(__file__), 'battle_data.py')

GEN1_VG = 'red-blue'
NUM_SPECIES = 151
NUM_MOVES = 165  # los ids 1..165 de PokeAPI son justo los movimientos de gen 1

# orden clasico de gen 1
TYPES = ['normal', 'fighting', 'flying', 'poison', 'ground', 'rock', 'bug', 'ghost',
         'fire', 'water', 'grass', 'electric', 'psychic', 'ice', 'dragon']

# special-attack / special-defense modernos son el unico Special de gen 1
STAT_GEN1 = {'attack': 'attack', 'defense': 'defense', 'speed': 'speed',
             'special-attack': 'special', 'special-defense': 'special',
             'accuracy': 'accuracy', 'evasion': 'evasion'}

# efectos que cambiaron desde gen 1 y que la API no guarda en past_values
GEN1_STONES = ('fire-stone', 'water-stone', 'thunder-stone', 'leaf-stone', 'moon-stone')

MOVE_FIXES = {
    'growth': {'stat_changes': [('special', 1)]},  # gen 1: solo Special +1
    'low-kick': {'flinch_chance': 30},  # gen 1: poder fijo 50 y 30 % de retroceso
}


def get(url):
    """GET con cache en disco. `url` puede ser relativa a la API."""
    if not url.startswith('http'):
        url = API + url
    rel = url.split('://', 1)[1].strip('/').replace('/', '_').replace('?', '_') + '.json'
    path = os.path.join(CACHE, rel)
    if os.path.exists(path):
        with open(path, encoding='utf-8') as fh:
            return json.load(fh)
    os.makedirs(CACHE, exist_ok=True)
    req = urllib.request.Request(url, headers={'User-Agent': 'poketrek-tools'})
    for intento in range(4):
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                data = json.load(resp)
            break
        except OSError:
            if intento == 3:
                raise
            time.sleep(2 ** intento)
    with open(path, 'w', encoding='utf-8') as fh:
        json.dump(data, fh)
    time.sleep(0.05)  # uso justo de PokeAPI
    return data


def name_in(entry, lang):
    for n in entry['names']:
        if n['language']['name'] == lang:
            return n['name']
    return None


def vg_order(name, _cache={}):
    if name not in _cache:
        _cache[name] = get(f'version-group/{name}')['order']
    return _cache[name]


def fetch_types():
    chart = {}
    names_es = {}
    for atk in TYPES:
        t = get(f'type/{atk}')
        names_es[atk] = name_in(t, 'es')
        rel = t['damage_relations']
        if t['past_damage_relations']:
            # entrada de la generacion mas antigua = la vigente en gen 1
            rel = min(t['past_damage_relations'], key=gen_num)['damage_relations']
        for dfn in TYPES:
            chart[(atk, dfn)] = 1.0
        for key, mult in (('double_damage_to', 2.0), ('half_damage_to', 0.5), ('no_damage_to', 0.0)):
            for d in rel[key]:
                if d['name'] in TYPES:
                    chart[(atk, d['name'])] = mult
    return chart, names_es


def gen_num(entry):
    return int(entry['generation']['url'].rstrip('/').rsplit('/', 1)[1])


def gen1_types(p):
    # cada past_types vale hasta su generacion (generation-v = de la 1 a la 5),
    # asi que la mas antigua es la de gen 1
    types = p['types']
    if p['past_types']:
        types = min(p['past_types'], key=gen_num)['types']
    names = [t['type']['name'] for t in sorted(types, key=lambda t: t['slot'])]
    return (names[0], names[1] if len(names) > 1 else None)


def fetch_moves():
    moves = {}
    for mid in range(1, NUM_MOVES + 1):
        m = get(f'move/{mid}')
        cur = {'type': m['type']['name'], 'power': m['power'], 'accuracy': m['accuracy'],
               'pp': m['pp'], 'effect_chance': m['effect_chance']}
        # past_values: valores anteriores al version group indicado; aplicados del
        # mas nuevo al mas viejo dejan los de gen 1
        for pv in sorted(m['past_values'], key=lambda p: vg_order(p['version_group']['name']), reverse=True):
            if vg_order(pv['version_group']['name']) <= vg_order(GEN1_VG):
                continue
            for k in ('power', 'accuracy', 'pp', 'effect_chance'):
                if pv[k] is not None:
                    cur[k] = pv[k]
            if pv['type'] is not None:
                cur['type'] = pv['type']['name']
        meta = m['meta'] or {}
        stats = []
        for sc in m['stat_changes']:
            st = (STAT_GEN1[sc['stat']['name']], sc['change'])
            if st not in stats:
                stats.append(st)
        mv = {
            'slug': m['name'],
            'en': name_in(m, 'en'),
            'es': name_in(m, 'es'),
            **cur,
            'priority': m['priority'],
            'target': m['target']['name'],
            'category': (meta.get('category') or {}).get('name', 'damage'),
            'ailment': (meta.get('ailment') or {}).get('name', 'none'),
            'ailment_chance': meta.get('ailment_chance') or 0,
            'flinch_chance': meta.get('flinch_chance') or 0,
            'crit_rate': meta.get('crit_rate') or 0,
            'drain': meta.get('drain') or 0,
            'healing': meta.get('healing') or 0,
            'min_hits': meta.get('min_hits') or 0,
            'max_hits': meta.get('max_hits') or 0,
            'min_turns': meta.get('min_turns') or 0,
            'max_turns': meta.get('max_turns') or 0,
            'stat_chance': meta.get('stat_chance') or 0,
            'stat_changes': stats,
        }
        mv.update(MOVE_FIXES.get(m['name'], {}))
        if cur['type'] not in TYPES:
            raise SystemExit(f"{m['name']}: tipo {cur['type']} no existe en gen 1")
        moves[mid] = mv
    return moves


def fetch_machines(moves):
    """TM/HM de red-blue: {'TM01': move_id, ...}"""
    machines = {}
    for mid in moves:
        m = get(f'move/{mid}')
        for mc in m['machines']:
            if mc['version_group']['name'] != GEN1_VG:
                continue
            item = get(mc['machine']['url'])['item']['name'].upper()  # tm01 / hm01
            machines[item] = mid
    return dict(sorted(machines.items(), key=lambda kv: (kv[0][:2] == 'HM', kv[0])))


def plain(det):
    """Metodo de gen 1: del version group red-blue (si lo indica) y sin
    condiciones extra (hora, region, felicidad...), que es como vienen las
    variantes posteriores (Raichu de Alola, Espeon, Slowbro de Galar)."""
    vg = det.get('version_group')
    if vg and vg['name'] != GEN1_VG:
        return False
    extra = {k: v for k, v in det.items()
             if k not in ('trigger', 'min_level', 'item', 'version_group', 'is_default',
                          'required_pokemon_form')
             and v not in (None, '', False, [])}
    return not extra


def walk_chain(node, out):
    src = int(node['species']['url'].rstrip('/').rsplit('/', 1)[1])
    for nxt in node['evolves_to']:
        dst = int(nxt['species']['url'].rstrip('/').rsplit('/', 1)[1])
        if src <= NUM_SPECIES and dst <= NUM_SPECIES:
            for det in nxt['evolution_details']:
                trig = det['trigger']['name']
                if not plain(det):
                    continue
                if trig == 'level-up' and det['min_level']:
                    out.add((src, dst, 'level', det['min_level']))
                elif trig == 'use-item' and det['item']['name'] in GEN1_STONES:
                    out.add((src, dst, 'item', det['item']['name']))
                elif trig == 'trade':
                    out.add((src, dst, 'trade', None))
        walk_chain(nxt, out)


def main():
    chart, type_es = fetch_types()
    moves = fetch_moves()
    machines = fetch_machines(moves)
    move_to_tm = {mid: tm for tm, mid in machines.items()}

    species, learnset, tm_compat, chains = {}, {}, {}, set()
    evos = set()
    for num in range(1, NUM_SPECIES + 1):
        p = get(f'pokemon/{num}')
        s = get(f'pokemon-species/{num}')
        base = {st['stat']['name']: st['base_stat'] for st in p['stats']}
        # cada past_stats vale hasta su generacion (generation-v: los de antes de
        # los cambios de gen 6, como la Defensa 30 de Pikachu); aplicados del mas
        # nuevo al mas viejo dejan los de gen 1, con su Special
        for past in sorted(p['past_stats'], key=gen_num, reverse=True):
            for st in past['stats']:
                base[st['stat']['name']] = st['base_stat']
        t1, t2 = gen1_types(p)
        species[num] = {
            'slug': p['name'],
            'types': (t1, t2),
            'hp': base['hp'], 'attack': base['attack'], 'defense': base['defense'],
            'speed': base['speed'], 'special': base['special'],
            'capture_rate': s['capture_rate'],
            'base_exp': p['base_experience'],
            'growth': s['growth_rate']['name'],
            'height_dm': p['height'],   # decimetros: tamano en pantalla
            'weight_hg': p['weight'],   # hectogramos
        }
        lv, tms = [], set()
        for mv in p['moves']:
            mid = int(mv['move']['url'].rstrip('/').rsplit('/', 1)[1])
            for d in mv['version_group_details']:
                if d['version_group']['name'] != GEN1_VG:
                    continue
                how = d['move_learn_method']['name']
                if how == 'level-up':
                    lv.append((d['level_learned_at'], mid))
                elif how == 'machine' and mid in move_to_tm:
                    tms.add(move_to_tm[mid])
        learnset[num] = sorted(set(lv))
        tm_compat[num] = sorted(tms, key=lambda tm: list(machines).index(tm))
        chain_url = s['evolution_chain']['url']
        if chain_url not in chains:
            chains.add(chain_url)
            walk_chain(get(chain_url)['chain'], evos)

    out = ['# GENERADO por tools/fetch_battle_data.py desde PokeAPI y PoGo API - no editar\n',
           '# ruff: noqa\n\n']
    for name, val in (('TYPES', TYPES), ('TYPE_NAMES_ES', type_es),
                      ('TYPE_CHART', {f'{a}>{d}': m for (a, d), m in chart.items() if m != 1.0}),
                      ('SPECIES', species), ('MOVES', moves), ('MACHINES', machines),
                      ('LEARNSET', learnset), ('TM_COMPAT', tm_compat),
                      ('EVOLUTIONS', sorted(evos, key=lambda e: (e[0], e[1])))):
        out.append(f'{name} = {pprint.pformat(val, width=110, sort_dicts=False)}\n\n')
    with open(OUT, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write(''.join(out))
    print(f'guardado {os.path.normpath(OUT)}: {len(species)} especies, {len(moves)} movimientos, '
          f'{len(machines)} MT/MO, {len(evos)} evoluciones')


if __name__ == '__main__':
    main()
