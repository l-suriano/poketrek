# PokéTrek para el Rabbit R1

Versión de PokéTrek como *creation* del Rabbit R1: una app web de 240×282
que se instala escaneando un QR. El juego es el mismo motor en C++ de la
placa (batallas, entrenador, exploración, tiendas y torneos), compilado a
WebAssembly. Esta carpeta no cambia ningún archivo del juego: solo los usa.

## Controles

| R1 | En el PC | Acción |
|---|---|---|
| Rueda | Flechas o rueda del ratón | Moverse por los menús |
| Botón lateral | Intro | Aceptar |
| Presión larga | Esc | Volver |
| Inclinar el R1 | Ratón | Apuntar la Poké Ball |
| Girar la rueda hacia arriba rápido | Clic sobre el Pokémon | Lanzar (2 pasos de rueda = se queda corta) |
| Pantalla táctil | Clic | Tocar |

El aro que se encoge da el bono del lanzamiento: Nice ×1.3, Great ×1.6,
Excellent ×2. Los pasos se cuentan con el acelerómetro mientras la app está
abierta; en *Explorar* se elige si caminas afuera o en casa (el R1 no sabe si
está en el Wi-Fi de casa) y hay un botón de pasos de prueba.

## Archivos

- `bridge.cpp`: funciones `pt_*` que la interfaz llama; devuelven JSON.
- `test_bridge.cpp`, `check_json.py`: prueba nativa que juega una partida.
- `web/`: la creation (`index.html`, `ui.js`, `screens.js`, `battle.js`,
  `text.js`, `game.js`, `sw.js` para jugar sin conexión).
- `tools/fetch_assets.py`: baja los sprites de PMD SpriteCollab y la fuente.
- `tools/dev_server.py`: prueba la interfaz en el PC con el motor nativo.

## Compilar y probar

```bash
make -C r1 test        # prueba nativa del puente (g++)
make -C r1 dev         # interfaz en http://localhost:8000 con el motor nativo
python3 r1/tools/fetch_assets.py   # sprites y fuente (una vez)
make -C r1 web         # motor a WebAssembly: web/poketrek.js (necesita emcc)
```

Para `make web` hace falta Emscripten:

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh   # en Windows: emsdk_env.bat
```

## Publicar e instalar en el R1

La creation se aloja en Cloudflare Pages (HTTPS, dirección difícil de
adivinar, sin enlaces en ningún lado). Con HTTPS el service worker guarda todo
la primera vez y después el R1 la abre sin internet.

```bash
cd r1 && npx wrangler pages deploy --branch main
```

`r1/wrangler.toml` dice el proyecto (`ptk-r1-t8k2mi`), la carpeta (`web`) y la
base D1 de la telemetria.

## Telemetria anonima

El juego manda errores y datos de uso (pantallas, combates, capturas,
acciones como compras o MTs, pasos, visitas y una foto del progreso al abrir
y al salir) a
`/api/t` (`r1/functions/api/t.js`), que los guarda en la base D1
`ptk-telemetry` (`r1/schema.sql`). No van nombres ni datos personales: solo
un identificador al azar del aparato. No hay forma publica de leerlos; se
consultan desde la cuenta con `r1/tools/telemetry.ps1` (errores, pantallas,
combates, hoy o una consulta SQL). El jugador lo apaga en Ajustes.

Después se genera un QR con la dirección (por ejemplo con `qr/` del
[creations-sdk](https://github.com/rabbit-hmi-oss/creations-sdk)) y se
escanea con el R1. Al cambiar el juego hay que subir `VERSION` en `web/sw.js`
para que el R1 baje la versión nueva.

## Respaldo en la nube

Opcional: el jugador lo activa en Ajustes > Respaldo en la nube. El juego crea
un código al azar de 10 caracteres (sin I, O, 0 ni 1) y sube la partida a
`/api/save` (`r1/functions/api/save.js`) como mucho cada 2 minutos y al salir
de la app, solo si cambió. Se guarda en la tabla `saves` de la misma base D1
(`r1/schema.sql`): la partida en base64 y los nombres del jugador y del rival.
El código es la llave: con él se restaura desde Ajustes o desde la primera
pantalla de un R1 sin partida ("Restore / Restaurar"). Todo va por POST; el
código nunca va en la dirección.

## Créditos

Sprites: [PMD SpriteCollab](https://sprites.pmdcollab.org/) (CC BY-NC 4.0).
Fuente: Press Start 2P (SIL OFL 1.1). Pokémon © Nintendo / Game Freak /
The Pokémon Company. Proyecto personal sin fines comerciales.
