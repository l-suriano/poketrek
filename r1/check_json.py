"""Comprueba que cada linea de la salida de test_bridge sea JSON valido."""
import json
import sys

bad = 0
lines = 0
with open(sys.argv[1], encoding='utf-8') as f:
    for n, line in enumerate(f, 1):
        tag, _, text = line.rstrip('\n').partition('\t')
        lines += 1
        try:
            json.loads(text)
        except json.JSONDecodeError as e:
            bad += 1
            if bad <= 5:
                print(f'linea {n} ({tag}): {e}\n  {text[:300]}')
print(f'{lines} respuestas, {bad} con JSON invalido')
sys.exit(1 if bad else 0)
