#!/usr/bin/env python3
"""Servidor de desarrollo de la version del R1, sin Emscripten.

Carga el puente compilado en nativo (build/bridge.dll o .so, `make -C r1 lib`)
y sirve r1/web/ en http://localhost:8000. game.js, si no encuentra
poketrek.js (WebAssembly), llama a /api/<funcion>?a=..&a=.. y este servidor
la ejecuta en el puente nativo. Asi se prueba la interfaz en el navegador del
PC con el mismo motor.

  python3 r1/tools/dev_server.py [puerto]
"""
import ctypes
import http.server
import os
import re
import sys
import urllib.parse

HERE = os.path.dirname(os.path.abspath(__file__))
R1 = os.path.join(HERE, '..')
WEB = os.path.join(R1, 'web')
LIB = os.path.join(R1, 'build', 'bridge.dll' if os.name == 'nt' else 'bridge.so')

lib = ctypes.CDLL(LIB)
SIGS = {}
for line in open(os.path.join(R1, 'bridge.cpp'), encoding='utf-8'):
    m = re.match(r'API (const char \*|int |void |uint8_t \*)(pt_\w+)\(([^)]*)\)', line)
    if not m:
        continue
    ret, name, params = m.groups()
    args = [p.strip() for p in params.split(',') if p.strip()]
    f = getattr(lib, name)
    f.restype = ctypes.c_char_p if ret.startswith('const char') else None if ret.startswith('void') else ctypes.c_int
    f.argtypes = [ctypes.c_char_p if 'char' in a else ctypes.c_uint32 if 'uint32' in a else ctypes.c_int for a in args]
    SIGS[name] = f


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **k):
        super().__init__(*a, directory=WEB, **k)

    def log_message(self, fmt, *args):
        if '/api/' not in self.path:
            super().log_message(fmt, *args)

    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()

    def do_GET(self):
        url = urllib.parse.urlparse(self.path)
        if url.path == '/poketrek.js':
            self.send_error(404)  # sin WebAssembly: game.js usa este servidor
            return
        if not url.path.startswith('/api/'):
            return super().do_GET()
        name = url.path[5:]
        f = SIGS.get(name)
        if f is None:
            self.send_error(404, name)
            return
        raw = urllib.parse.parse_qs(url.query, keep_blank_values=True).get('a', [])
        args = [v.encode() if t is ctypes.c_char_p else int(v) for v, t in zip(raw, f.argtypes)]
        res = f(*args)
        body = (res if isinstance(res, bytes) else b'' if res is None else str(res).encode())
        self.send_response(200)
        self.send_header('Content-Type', 'text/plain; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)


if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    print(f'PokeTrek R1 (puente nativo) en http://localhost:{port}')
    http.server.ThreadingHTTPServer(('127.0.0.1', port), Handler).serve_forever()
