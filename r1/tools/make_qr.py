#!/usr/bin/env python3
"""Genera el icono de la creation (una Poke Ball dibujada aqui) y el QR que
escanea el R1 para instalarla.

El QR lleva el JSON que usa el generador del creations-sdk de Rabbit:
{title, url, description, iconUrl, themeColor}.

  python3 r1/tools/make_qr.py https://<proyecto>.pages.dev
Necesita Pillow y qrcode (pip install qrcode). Deja web/icon.png y build/qr.png.
"""
import json
import os
import sys

import qrcode
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
R1 = os.path.join(HERE, '..')


def icon(path, size=128):
    img = Image.new('RGBA', (size, size), (16, 20, 32, 255))
    d = ImageDraw.Draw(img)
    m, c = 12, size // 2
    d.ellipse((m, m, size - m, size - m), fill=(232, 80, 58), outline=(40, 40, 54), width=6)
    d.pieslice((m, m, size - m, size - m), 0, 180, fill=(248, 248, 248), outline=(40, 40, 54), width=6)
    d.rectangle((m, c - 4, size - m, c + 4), fill=(40, 40, 54))
    d.ellipse((c - 16, c - 16, c + 16, c + 16), fill=(248, 248, 248), outline=(40, 40, 54), width=6)
    d.ellipse((c - 6, c - 6, c + 6, c + 6), fill=(216, 216, 224))
    img.save(path)


def main():
    url = sys.argv[1].rstrip('/') + '/'
    icon(os.path.join(R1, 'web', 'icon.png'))
    data = {
        'title': 'PokéTrek',
        'url': url,
        'description': 'Walk, catch and battle: Gen 1 adventure',
        'iconUrl': url + 'icon.png',
        'themeColor': '#E8503A',
    }
    os.makedirs(os.path.join(R1, 'build'), exist_ok=True)
    qr = qrcode.QRCode(border=4, box_size=10, error_correction=qrcode.constants.ERROR_CORRECT_M)
    qr.add_data(json.dumps(data))  # en ASCII (é): el lector del R1 no tiene que adivinar la codificacion
    qr.make(fit=True)
    out = os.path.join(R1, 'build', 'qr.png')
    qr.make_image(fill_color='black', back_color='white').save(out)
    print(json.dumps(data, ensure_ascii=False))
    print(out)


if __name__ == '__main__':
    main()
