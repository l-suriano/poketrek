'use strict';
// Mundo de Kanto en pixel art al estilo de FireRed/LeafGreen, dibujado por
// codigo una sola vez en un lienzo grande (WORLD_W x WORLD_H) y la camara
// muestra la parte que toca. Pinos, pasto, caminos de tierra, agua con orilla,
// montes rocosos y cada ciudad como un pueblito con su edificio iconico.

const WK = 2.8, WOFF = 18;              // del mapa chico (x, y) al mundo
const WORLD_W = 700, WORLD_H = 590;
const wx = c => Math.round(c.x * WK + WOFF);
const wy = c => Math.round(c.y * WK + WOFF);
let worldCanvas = null;

// tierra (en coordenadas del mapa chico, como antes) e isla Canela
const LAND = [[6, 12], [234, 12], [234, 142], [178, 142], [164, 178], [96, 178], [84, 148], [6, 148]];

function inPoly(x, y, pts) {
  let inside = false;
  for (let i = 0, j = pts.length - 1; i < pts.length; j = i++) {
    const [xi, yi] = pts[i], [xj, yj] = pts[j];
    if ((yi > y) !== (yj > y) && x < (xj - xi) * (y - yi) / (yj - yi) + xi) inside = !inside;
  }
  return inside;
}
function segDist(px, py, ax, ay, bx, by) {
  const dx = bx - ax, dy = by - ay;
  const t = Math.max(0, Math.min(1, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy || 1)));
  return Math.hypot(px - ax - t * dx, py - ay - t * dy);
}

// montes: Mt. Moon, Tunel Roca y Calle Victoria (en coordenadas del mundo)
const MOUNTS = [[290, 108, 46], [596, 150, 38], [70, 180, 34]];

function buildWorld() {
  const c = document.createElement('canvas');
  c.width = WORLD_W; c.height = WORLD_H;
  const g = c.getContext('2d');
  g.imageSmoothingEnabled = false;
  const R = (x, y, w, h, col) => { g.fillStyle = col; g.fillRect(Math.round(x), Math.round(y), w, h); };
  const landW = LAND.map(([x, y]) => [x * WK + WOFF, y * WK + WOFF]);
  const cin = { x: wx(CITIES[9]), y: wy(CITIES[9]) };
  const isLand = (x, y) => inPoly(x, y, landW) || Math.hypot(x - cin.x, y - cin.y) < 40;

  // agua y tierra por casillas de 8 px (orilla clara donde se tocan)
  for (let y = 0; y < WORLD_H; y += 8) for (let x = 0; x < WORLD_W; x += 8) {
    const land = isLand(x + 4, y + 4);
    if (land) {
      R(x, y, 8, 8, '#88d070');
      if (hash(x, y) < 0.25) { R(x + 2, y + 3, 1, 2, '#70b858'); R(x + 4, y + 4, 1, 2, '#70b858'); }
    } else {
      R(x, y, 8, 8, '#4890e8');
      if (hash(x + 3, y) < 0.3) R(x + 1, y + 3, 5, 1, '#78b8f8');
      // orilla: arena y espuma si hay tierra al lado
      if (isLand(x + 4, y - 4) || isLand(x + 4, y + 12) || isLand(x - 4, y + 4) || isLand(x + 12, y + 4)) {
        R(x, y, 8, 8, '#68a8f0');
        R(x + 1, y + 1, 6, 1, '#d8f0ff');
      }
    }
  }
  // caminos de tierra entre ciudades
  const pathPts = PATHS.map(([a, b]) => [wx(CITIES[a]), wy(CITIES[a]), wx(CITIES[b]), wy(CITIES[b])]);
  for (const [ax, ay, bx, by] of pathPts) {
    g.lineCap = 'round';
    g.strokeStyle = '#c0a060'; g.lineWidth = 14; g.beginPath(); g.moveTo(ax, ay); g.lineTo(bx, by); g.stroke();
    g.strokeStyle = '#e8d8a0'; g.lineWidth = 10; g.beginPath(); g.moveTo(ax, ay); g.lineTo(bx, by); g.stroke();
  }
  // ruta por mar (puntos blancos)
  g.setLineDash([4, 6]); g.strokeStyle = '#e8f6ff'; g.lineWidth = 3;
  for (const [a, b] of SEA_PATHS) { g.beginPath(); g.moveTo(wx(CITIES[a]), wy(CITIES[a])); g.lineTo(wx(CITIES[b]), wy(CITIES[b])); g.stroke(); }
  g.setLineDash([]);
  // montes
  for (const [mx, my, r] of MOUNTS) drawMountain(g, mx, my, r);
  // pinos donde no hay camino, ciudad, monte ni agua
  for (let y = 4; y < WORLD_H - 20; y += 14) for (let x = (y / 14) % 2 * 8; x < WORLD_W - 8; x += 16) {
    const cx = x + 8, cy = y + 14;
    if (!isLand(cx, cy) || !isLand(cx, cy - 12)) continue;
    if (pathPts.some(([ax, ay, bx, by]) => segDist(cx, cy, ax, ay, bx, by) < 18)) continue;
    if (CITIES.some(ci => Math.abs(cx - wx(ci)) < 52 && Math.abs(cy - wy(ci)) < 44)) continue;
    if (MOUNTS.some(([mx, my, r]) => Math.hypot(cx - mx, cy - my) < r + 8)) continue;
    if (hash(x, y + 7) < 0.12) { drawFlowers(g, x, y + 8); continue; }
    drawPine(g, x, y);
  }
  // pueblos
  CITIES.forEach(ci => drawTown(g, ci));
  worldCanvas = c;
}

function drawPine(g, x, y) {
  g.fillStyle = 'rgba(0,0,0,0.18)'; g.fillRect(x + 3, y + 19, 11, 3);
  g.fillStyle = '#704828'; g.fillRect(x + 6, y + 15, 4, 5);
  const tri = (ax, ay, bx, by, cx, cy, col) => { g.fillStyle = col; g.beginPath(); g.moveTo(ax, ay); g.lineTo(bx, by); g.lineTo(cx, cy); g.closePath(); g.fill(); };
  tri(x - 1, y + 17, x + 17, y + 17, x + 8, y + 5, '#1c4a28');
  tri(x + 0, y + 16, x + 16, y + 16, x + 8, y + 6, '#2f7a3a');
  tri(x + 1, y + 11, x + 15, y + 11, x + 8, y + 1, '#1c4a28');
  tri(x + 2, y + 10, x + 14, y + 10, x + 8, y + 2, '#3f9448');
  tri(x + 4, y + 6, x + 12, y + 6, x + 8, y - 1, '#58b050');
  g.fillStyle = '#80d070'; g.fillRect(x + 6, y + 3, 2, 2); g.fillRect(x + 4, y + 8, 2, 2); g.fillRect(x + 3, y + 13, 2, 2);
}

function drawFlowers(g, x, y) {
  const col = hash(x, y) < 0.5 ? '#f85858' : '#f8f8f8';
  [[3, 2], [9, 5], [5, 8]].forEach(([dx, dy]) => { g.fillStyle = col; g.fillRect(x + dx, y + dy, 3, 3); g.fillStyle = '#f8d838'; g.fillRect(x + dx + 1, y + dy + 1, 1, 1); });
}

function drawMountain(g, mx, my, r) {
  for (let i = 0; i < 4; i++) {
    const x = mx + (i - 1.5) * r * 0.55, s = r * (0.7 + (i % 2) * 0.35);
    g.fillStyle = '#806040'; g.beginPath(); g.moveTo(x - s, my + r * 0.4); g.lineTo(x, my - s); g.lineTo(x + s, my + r * 0.4); g.fill();
    g.fillStyle = '#a88860'; g.beginPath(); g.moveTo(x - s * 0.6, my + r * 0.2); g.lineTo(x, my - s); g.lineTo(x + s * 0.15, my + r * 0.3); g.fill();
    g.fillStyle = '#c8b088'; g.beginPath(); g.moveTo(x - s * 0.25, my - s * 0.6); g.lineTo(x, my - s); g.lineTo(x + s * 0.1, my - s * 0.7); g.fill();
  }
  // entrada de la cueva
  g.fillStyle = '#281c14'; g.beginPath(); g.ellipse(mx, my + r * 0.3, 6, 5, 0, Math.PI, 0); g.fill();
}

// casa de los juegos: techo con franjas, pared clara, ventanas y puerta
function drawHouse(g, x, y, w, h, roof, roofDark, wall = '#e8e0d0') {
  const rh = Math.round(h * 0.45);
  g.fillStyle = 'rgba(0,0,0,0.2)'; g.fillRect(x + 2, y + h, w, 3);
  g.fillStyle = wall; g.fillRect(x + 1, y + rh, w - 2, h - rh);
  g.fillStyle = '#a8a098'; g.fillRect(x + 1, y + h - 2, w - 2, 2);
  g.fillStyle = roofDark; g.fillRect(x - 1, y, w + 2, rh + 1);
  g.fillStyle = roof; g.fillRect(x, y, w, rh - 1);
  g.fillStyle = roofDark; for (let i = y + 3; i < y + rh - 1; i += 3) g.fillRect(x, i, w, 1);
  const wins = Math.max(1, Math.floor((w - 8) / 9));
  for (let i = 0; i < wins; i++) { g.fillStyle = '#5078a8'; g.fillRect(x + 4 + i * 9, y + rh + 3, 5, 4); g.fillStyle = '#a8d0f0'; g.fillRect(x + 4 + i * 9, y + rh + 3, 5, 1); }
  g.fillStyle = '#704828'; g.fillRect(x + Math.floor(w / 2) - 2, y + h - 7, 5, 7);
}

function drawTown(g, ci) {
  const cx = wx(ci), cy = wy(ci);
  // plaza con valla
  g.fillStyle = '#b8a070'; g.fillRect(cx - 47, cy - 37, 94, 76);
  g.fillStyle = '#d8e8b0'; g.fillRect(cx - 45, cy - 35, 90, 72);
  g.fillStyle = '#c8d8a0'; for (let x = cx - 45; x < cx + 45; x += 6) g.fillRect(x, cy - 35, 1, 72);
  g.fillStyle = '#a07850';
  for (let x = cx - 46; x <= cx + 44; x += 5) { g.fillRect(x, cy - 37, 2, 3); g.fillRect(x, cy + 36, 2, 3); }
  // Centro Pokemon (techo rojo) y Poke Mart (techo azul), salvo en Paleta
  if (!ci.lab) {
    drawHouse(g, cx - 42, cy + 8, 24, 22, '#e85848', '#a83028');
    g.fillStyle = '#f8f8f8'; g.fillRect(cx - 33, cy + 2 + 8, 6, 3);
    drawHouse(g, cx + 18, cy + 10, 22, 20, '#4878d8', '#2850a0');
  }
  drawLandmark(g, ci, cx, cy);
}

// el edificio que identifica cada ciudad
function drawLandmark(g, ci, cx, cy) {
  const R = (x, y, w, h, col) => { g.fillStyle = col; g.fillRect(Math.round(x), Math.round(y), w, h); };
  switch (ci.icon) {
    case 'lab':  // Laboratorio de Oak y las casas roja y azul
      drawHouse(g, cx - 18, cy + 4, 40, 26, '#a8b0b8', '#787880', '#f0f0f0');
      drawHouse(g, cx - 42, cy - 30, 26, 24, '#e85848', '#a83028');
      drawHouse(g, cx + 16, cy - 30, 26, 24, '#4878d8', '#2850a0');
      break;
    case 'museum':  // Museo de Plateada
      R(cx - 26, cy - 30, 52, 30, '#a89078'); R(cx - 28, cy - 34, 56, 6, '#786048');
      for (let i = 0; i < 5; i++) R(cx - 22 + i * 10, cy - 26, 4, 24, '#e0d8c8');
      R(cx - 4, cy - 10, 8, 10, '#704828');
      break;
    case 'bluegym':  // gimnasio de Celeste, techo azul
      drawHouse(g, cx - 22, cy - 32, 44, 32, '#58a8e8', '#3070b8', '#e8f0f8');
      R(cx - 6, cy - 30, 12, 4, '#f8f8f8');
      break;
    case 'silph':  // Torre de Silph
      R(cx - 14, cy - 44, 28, 50, '#a0a8b8'); R(cx - 14, cy - 44, 28, 3, '#707888');
      for (let y = cy - 38; y < cy + 2; y += 6) for (let x = cx - 11; x < cx + 12; x += 6) R(x, y, 4, 3, '#4868a0');
      R(cx - 4, cy - 2, 8, 8, '#505868');
      break;
    case 'dept':  // Centro Comercial de Azulona
      R(cx - 22, cy - 40, 44, 44, '#f0f0e8'); R(cx - 22, cy - 40, 44, 4, '#c03028');
      for (let y = cy - 32; y < cy; y += 7) R(cx - 19, y, 38, 4, '#78a8d8');
      R(cx - 5, cy - 3, 10, 7, '#704828');
      break;
    case 'ship':  // S.S. Anne en el puerto de Carmin
      R(cx - 44, cy - 34, 30, 22, '#4890e8');
      R(cx - 42, cy - 28, 26, 9, '#f8f8f8'); R(cx - 42, cy - 21, 26, 3, '#c03028'); R(cx - 36, cy - 34, 10, 6, '#f8f8f8');
      R(cx - 33, cy - 40, 4, 6, '#e8b030');
      drawHouse(g, cx - 4, cy - 30, 34, 28, '#e8a848', '#b07020');  // gimnasio
      break;
    case 'tower':  // Torre Pokemon de Lavanda
      R(cx - 10, cy - 46, 20, 48, '#9070b8'); R(cx - 12, cy - 50, 24, 6, '#604088');
      for (let y = cy - 40; y < cy - 2; y += 8) R(cx - 6, y, 12, 3, '#d0c0e8');
      R(cx - 3, cy - 4, 6, 6, '#382848');
      drawHouse(g, cx + 16, cy - 28, 24, 22, '#9070b8', '#604088');
      break;
    case 'safari':  // entrada de la Zona Safari
      R(cx - 30, cy - 34, 60, 6, '#a07040'); R(cx - 28, cy - 28, 6, 26, '#806030'); R(cx + 22, cy - 28, 6, 26, '#806030');
      R(cx - 20, cy - 26, 40, 22, '#e8d8b0'); R(cx - 6, cy - 14, 12, 12, '#704828');
      R(cx - 24, cy - 38, 48, 5, '#58a850');
      break;
    case 'volcano':  // volcan de Canela
      g.fillStyle = '#805038'; g.beginPath(); g.moveTo(cx - 34, cy); g.lineTo(cx - 8, cy - 40); g.lineTo(cx + 8, cy - 40); g.lineTo(cx + 34, cy); g.fill();
      R(cx - 8, cy - 42, 16, 4, '#f86030'); R(cx - 4, cy - 46, 8, 4, '#f8a030');
      break;
    case 'league':  // edificio de la Liga en la Meseta Anil
      R(cx - 30, cy - 34, 60, 34, '#f0e8d0'); R(cx - 32, cy - 40, 64, 8, '#8858c0'); R(cx - 6, cy - 50, 12, 12, '#e8c040');
      for (let i = 0; i < 6; i++) R(cx - 26 + i * 10, cy - 28, 4, 24, '#c8b890');
      R(cx - 5, cy - 12, 10, 12, '#604088');
      break;
    default:  // gimnasio gris y casas
      drawHouse(g, cx - 22, cy - 32, 44, 30, '#909098', '#606068', '#e0e0e0');
      drawHouse(g, cx + 26, cy - 30, 16, 18, '#58a850', '#387830');
  }
}
