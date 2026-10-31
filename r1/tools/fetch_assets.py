#!/usr/bin/env python3
"""Baja los recursos de la version del R1 a r1/web/assets/.

- Sprites de PMD SpriteCollab (CC BY-NC 4.0) para los 151: Idle, Attack y Hurt,
  de espaldas (fila 3, el tuyo, mirando arriba a la derecha) y de frente
  (fila 7, el rival, mirando abajo a la izquierda). Un PNG por especie con
  todas las tiras una debajo de otra y sprites.json con sus medidas.
- La fuente Press Start 2P (SIL OFL 1.1) de google/fonts.

  python3 r1/tools/fetch_assets.py          # los 151
  python3 r1/tools/fetch_assets.py 25 143   # algunas especies
Necesita Pillow. Guarda lo bajado en r1/tools/cache/ para no repetirlo.
"""
import io
import json
import os
import sys
import urllib.request
import xml.etree.ElementTree as ET

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, '..', 'web', 'assets')
CACHE = os.path.join(HERE, 'cache')
BASE = 'https://raw.githubusercontent.com/PMDCollab/SpriteCollab/master/sprite'
FONT = 'https://raw.githubusercontent.com/google/fonts/main/ofl/pressstart2p/PressStart2P-Regular.ttf'
FONT_LICENSE = 'https://raw.githubusercontent.com/google/fonts/main/ofl/pressstart2p/OFL.txt'
ANIMS = ('Idle', 'Attack', 'Hurt')
ROWS = {'back': 3, 'front': 7}


def get(url):
    path = os.path.join(CACHE, url.split('://', 1)[1].replace('/', '_'))
    if os.path.exists(path):
        return open(path, 'rb').read()
    req = urllib.request.Request(url, headers={'User-Agent': 'PokeTrek-R1-assets'})
    data = urllib.request.urlopen(req, timeout=30).read()
    os.makedirs(CACHE, exist_ok=True)
    open(path, 'wb').write(data)
    return data


def anim_info(root, name):
    anims = {a.find('Name').text: a for a in root.find('Anims')}
    a = anims.get(name)
    if a is None:
        return None
    if a.find('FrameWidth') is None and a.find('CopyOf') is not None:
        name = a.find('CopyOf').text
        a = anims[name]
    fw, fh = int(a.find('FrameWidth').text), int(a.find('FrameHeight').text)
    durs = [int(x.text) for x in a.find('Durations')]
    return name, fw, fh, durs


def species(dex):
    d = f'{dex:04d}'
    root = ET.fromstring(get(f'{BASE}/{d}/AnimData.xml'))
    strips = []
    meta = {}
    for anim in ANIMS:
        info = anim_info(root, anim)
        if info is None:
            continue
        src, fw, fh, durs = info
        sheet = Image.open(io.BytesIO(get(f'{BASE}/{d}/{src}-Anim.png'))).convert('RGBA')
        rows = sheet.height // fh
        for side, row in ROWS.items():
            r = row if rows > row else 0
            strip = sheet.crop((0, r * fh, fw * len(durs), (r + 1) * fh))
            meta[f'{anim}_{side}'] = {'y': sum(s.height for s in strips), 'w': fw, 'h': fh, 'd': durs}
            strips.append(strip)
    width = max(s.width for s in strips)
    atlas = Image.new('RGBA', (width, sum(s.height for s in strips)))
    y = 0
    for s in strips:
        atlas.paste(s, (0, y))
        y += s.height
    os.makedirs(os.path.join(OUT, 'sprites'), exist_ok=True)
    atlas.save(os.path.join(OUT, 'sprites', f'{dex}.png'), optimize=True)
    return meta


def main():
    wanted = [int(a) for a in sys.argv[1:]] or list(range(1, 152))
    manifest_path = os.path.join(OUT, 'sprites.json')
    manifest = json.load(open(manifest_path)) if os.path.exists(manifest_path) else {}
    for dex in wanted:
        try:
            manifest[str(dex)] = species(dex)
            print(dex, 'ok')
        except Exception as e:  # una especie que falle no para las demas
            print(dex, 'ERROR', e)
    os.makedirs(OUT, exist_ok=True)
    json.dump(manifest, open(manifest_path, 'w'), separators=(',', ':'), sort_keys=True)
    open(os.path.join(OUT, 'PressStart2P-Regular.ttf'), 'wb').write(get(FONT))
    open(os.path.join(OUT, 'OFL.txt'), 'wb').write(get(FONT_LICENSE))
    print(len(manifest), 'especies en sprites.json')


if __name__ == '__main__':
    main()
