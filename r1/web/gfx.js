'use strict';
// Graficos al estilo de los juegos de Game Boy Advance (FireRed / LeafGreen),
// dibujados por codigo: nada de imagenes oficiales. Fondos de combate por
// lugar, plataformas, recuadros redondeados, el mapa del inicio y la
// transicion al entrar en combate.

// recuadros: los de los juegos son claros con borde oscuro y un filo de color
const BOX = {
  light: { border: '#405060', rim: '#a8c0d0', fill: '#f8f8f8' },
  info: { border: '#404848', rim: '#d8d0a8', fill: '#f8f8e0' },
  dark: { border: '#202838', rim: '#4870a0', fill: '#283850' },
  sel: { border: '#c03028', rim: '#f8a088', fill: '#fff8f0' },
};
const INK = '#383840', INK_SHADOW = '#d0d0c8', INK_DIM = '#808890';

// recuadro con esquinas recortadas en pixeles
function roundBox(x, y, w, h, s = BOX.light) {
  x = Math.round(x); y = Math.round(y);
  rect(x + 2, y, w - 4, h, s.border);
  rect(x, y + 2, w, h - 4, s.border);
  rect(x + 1, y + 1, w - 2, h - 2, s.border);
  rect(x + 2, y + 1, w - 4, h - 2, s.rim);
  rect(x + 1, y + 2, w - 2, h - 4, s.rim);
  rect(x + 3, y + 3, w - 6, h - 6, s.fill);
  rect(x + 4, y + 2, w - 8, 1, s.fill);
  rect(x + 4, y + h - 3, w - 8, 1, s.fill);
}

// texto con la sombra gris de los juegos
function textS(s, x, y, color = INK, align = 'left', shadow = INK_SHADOW) {
  text(s, x + 1, y + 1, shadow, align);
  text(s, x, y, color, align);
}

// letras de 3x5 pixeles para etiquetas pequenas (HP, EXP)
const TINY = {
  H: ['101', '101', '111', '101', '101'], P: ['110', '101', '110', '100', '100'],
  E: ['111', '100', '110', '100', '111'], X: ['101', '101', '010', '101', '101'],
};
function tinyText(s, x, y, color) {
  ctx.fillStyle = color;
  let cx = Math.round(x);
  for (const ch of s) {
    const g = TINY[ch];
    if (g) g.forEach((row, r) => { for (let c = 0; c < 3; c++) if (row[c] === '1') ctx.fillRect(cx + c, Math.round(y) + r, 1, 1); });
    cx += 4;
  }
}

// cursor triangular ▶
function cursor(x, y, color = INK) {
  ctx.fillStyle = color;
  for (let i = 0; i < 4; i++) ctx.fillRect(x + i, y + i, 1, 8 - i * 2);
}

// franjas con trama de puntos (degradados de 16 colores como en GBA)
function bands(y0, y1, colors) {
  const n = colors.length;
  const h = (y1 - y0) / n;
  for (let i = 0; i < n; i++) {
    const y = Math.round(y0 + i * h), yh = Math.round(y0 + (i + 1) * h);
    rect(0, y, W, yh - y, colors[i]);
    if (i + 1 < n) {  // trama en el borde entre franjas
      ctx.fillStyle = colors[i + 1];
      for (let x = (i % 2); x < W; x += 2) ctx.fillRect(x, yh - 1, 1, 1);
    }
  }
}

// pseudo-azar estable para colocar detalles siempre igual
function hash(a, b = 0) {
  let h = (a * 374761393 + b * 668265263) | 0;
  h = (h ^ (h >>> 13)) * 1274126177;
  return ((h ^ (h >>> 16)) >>> 0) / 4294967296;
}

function stars(y0, y1, t) {
  for (let i = 0; i < 26; i++) {
    const x = Math.floor(hash(i, 1) * W), y = Math.floor(y0 + hash(i, 2) * (y1 - y0));
    if ((Math.floor(t / 500) + i) % 7) rect(x, y, 1, 1, i % 3 ? '#f8f8f8' : '#f8e890');
  }
}

// arboles lejanos: copas redondas en fila
function treeLine(y, color, dark, size = 10) {
  for (let x = -size; x < W + size; x += size * 1.4) {
    const r = size * (0.8 + hash(Math.floor(x), 7) * 0.5);
    ctx.fillStyle = dark;
    ctx.beginPath(); ctx.arc(Math.round(x), y + 2, r, Math.PI, 0); ctx.fill();
    ctx.fillStyle = color;
    ctx.beginPath(); ctx.arc(Math.round(x) - 1, y, r - 2, Math.PI, 0); ctx.fill();
  }
  rect(0, y, W, 3, dark);
}

// ------------------------------------------------------- fondos de combate --
// kind: grass | forest | cave | water | gym | league | stadium

const BG_SKY_DAY = ['#78b8f0', '#88c4f0', '#98d0f0', '#a8d8f0', '#b8e0f0'];
const BG_SKY_NIGHT = ['#101830', '#182040', '#202850', '#283060', '#303868'];

function drawBattleBg(kind, t, night, h = 176, accent) {
  switch (kind) {
    case 'forest': {
      bands(0, h * 0.45, night ? ['#081810', '#0c2018', '#102820'] : ['#307838', '#3c8840', '#489848']);
      treeLine(h * 0.42, night ? '#18402a' : '#58a850', night ? '#0c2818' : '#387838', 14);
      bands(h * 0.42, h, night ? ['#203820', '#284828', '#305030'] : ['#78b858', '#80c060', '#88c868', '#90d070']);
      if (!night) {  // rayos de luz entre las hojas
        ctx.fillStyle = 'rgba(255,255,220,0.12)';
        for (let i = 0; i < 3; i++) { ctx.beginPath(); ctx.moveTo(40 + i * 70, 0); ctx.lineTo(70 + i * 70, 0); ctx.lineTo(30 + i * 70, h); ctx.lineTo(10 + i * 70, h); ctx.fill(); }
      }
      break;
    }
    case 'cave': {
      bands(0, h * 0.5, ['#382820', '#403028', '#483830', '#504038']);
      // estalactitas
      ctx.fillStyle = '#281c18';
      for (let i = 0; i < 12; i++) {
        const x = hash(i, 3) * W, w = 6 + hash(i, 4) * 10, len = 10 + hash(i, 5) * 26;
        ctx.beginPath(); ctx.moveTo(x - w / 2, 0); ctx.lineTo(x + w / 2, 0); ctx.lineTo(x, len); ctx.fill();
      }
      bands(h * 0.5, h, ['#706050', '#7c6c58', '#887860', '#948468']);
      for (let i = 0; i < 18; i++) rect(Math.floor(hash(i, 8) * W), Math.floor(h * 0.55 + hash(i, 9) * h * 0.45), 3, 2, '#605040');
      ctx.fillStyle = 'rgba(0,0,0,0.25)';  // vineta
      ctx.fillRect(0, 0, 8, h); ctx.fillRect(W - 8, 0, 8, h);
      break;
    }
    case 'water': {
      bands(0, h * 0.4, night ? BG_SKY_NIGHT : BG_SKY_DAY);
      if (night) stars(0, h * 0.35, t);
      bands(h * 0.4, h, night ? ['#183060', '#1c3870', '#204078', '#244880'] : ['#3878d8', '#4088e0', '#4898e8', '#50a0f0']);
      ctx.fillStyle = night ? '#4060a0' : '#a8d8f8';
      for (let row = 0; row < 8; row++) {
        const y = Math.round(h * 0.44 + row * (h * 0.07));
        for (let x = ((t / 40 + row * 23) % 32) - 32; x < W; x += 32) ctx.fillRect(Math.round(x), y, 10, 1);
      }
      break;
    }
    case 'gym':
    case 'stadium': {
      const c = accent || '#c8a060';
      bands(0, h * 0.42, kind === 'stadium' ? ['#304060', '#384868', '#405070'] : ['#605868', '#686070', '#706878']);
      if (kind === 'stadium') {  // gradas con publico
        for (let r = 0; r < 4; r++) for (let x = 0; x < W; x += 6) {
          rect(x + (r % 2) * 3, 8 + r * 12, 4, 5, ['#e86050', '#f8d050', '#68a8f0', '#f8f8f8'][(x / 6 + r) % 4 | 0]);
        }
      } else {
        for (let x = 10; x < W; x += 48) { rect(x, 6, 10, h * 0.36, '#807888'); rect(x + 2, 6, 2, h * 0.36, '#9890a0'); }
      }
      rect(0, Math.round(h * 0.42), W, 3, '#383040');
      // suelo en damero con el color del gimnasio
      for (let y = Math.round(h * 0.42) + 3; y < h; y += 12) for (let x = 0; x < W; x += 16) {
        rect(x, y, 16, 12, ((x / 16 + (y / 12 | 0)) % 2) ? c : shade(c, -18));
      }
      break;
    }
    case 'league': {
      bands(0, h * 0.42, ['#201028', '#281430', '#301838', '#381c40']);
      for (let x = 14; x < W; x += 56) { rect(x, 0, 12, h * 0.42, '#583868'); rect(x + 2, 0, 3, h * 0.42, '#785090'); }
      for (let y = Math.round(h * 0.42); y < h; y += 10) for (let x = 0; x < W; x += 20) {
        rect(x, y, 20, 10, ((x / 20 + (y / 10 | 0)) % 2) ? '#604878' : '#503868');
      }
      break;
    }
    default: {  // pasto de una ruta
      bands(0, h * 0.42, night ? BG_SKY_NIGHT : BG_SKY_DAY);
      if (night) stars(0, h * 0.36, t);
      else for (let i = 0; i < 3; i++) cloud((hash(i, 11) * W + t / (60 + i * 30)) % (W + 60) - 30, 10 + i * 14);
      treeLine(h * 0.42, night ? '#284830' : '#60b058', night ? '#183020' : '#408840', 9);
      bands(h * 0.42, h, night ? ['#305030', '#345834', '#386038', '#3c683c'] : ['#88c860', '#90d068', '#98d870', '#a0e078']);
      ctx.fillStyle = night ? '#2c4c2c' : '#78b850';
      for (let i = 0; i < 40; i++) {
        const x = Math.floor(hash(i, 12) * W), y = Math.floor(h * 0.46 + hash(i, 13) * h * 0.52);
        ctx.fillRect(x, y, 1, 2); ctx.fillRect(x + 2, y + 1, 1, 2);
      }
    }
  }
}

function cloud(x, y) {
  ctx.fillStyle = '#f8f8f8';
  ctx.beginPath();
  ctx.arc(x, y + 4, 6, 0, Math.PI * 2); ctx.arc(x + 8, y + 2, 8, 0, Math.PI * 2); ctx.arc(x + 17, y + 4, 6, 0, Math.PI * 2);
  ctx.fill();
  rect(Math.round(x - 6), Math.round(y + 6), 29, 4, '#f8f8f8');
}

function shade(hex, d) {
  const n = parseInt(hex.slice(1), 16);
  const c = k => Math.max(0, Math.min(255, ((n >> k) & 255) + d));
  return '#' + ((c(16) << 16) | (c(8) << 8) | c(0)).toString(16).padStart(6, '0');
}

// plataforma ovalada bajo cada Pokemon
function platform(cx, cy, rx, ry, kind, night) {
  const P = {
    grass: ['#70c048', '#509830', '#90d868'], forest: ['#68a848', '#488830', '#80c060'],
    cave: ['#a89070', '#7c6850', '#c0a888'], water: ['#88c8f0', '#5898d8', '#b0e0f8'],
    gym: ['#d0d0d8', '#9898a8', '#e8e8f0'], stadium: ['#d0d0d8', '#9898a8', '#e8e8f0'],
    league: ['#9070a8', '#684880', '#b090c8'],
  }[kind] || ['#70c048', '#509830', '#90d868'];
  const [base, dark, light] = night && (kind === 'grass' || kind === 'forest') ? ['#3c6c34', '#2c5024', '#4c7c44'] : P;
  ctx.fillStyle = dark;
  ctx.beginPath(); ctx.ellipse(cx, cy + 2, rx, ry, 0, 0, Math.PI * 2); ctx.fill();
  ctx.fillStyle = base;
  ctx.beginPath(); ctx.ellipse(cx, cy, rx, ry, 0, 0, Math.PI * 2); ctx.fill();
  ctx.fillStyle = light;
  ctx.beginPath(); ctx.ellipse(cx - rx * 0.2, cy - ry * 0.3, rx * 0.55, ry * 0.4, 0, 0, Math.PI * 2); ctx.fill();
}

// pasto alto delante (encuentros)
function tallGrass(y, t, night) {
  const c1 = night ? '#2c5c2c' : '#40a040', c2 = night ? '#3c7038' : '#58b850', c3 = night ? '#204820' : '#308030';
  for (let x = -4; x < W + 8; x += 8) {
    const sway = Math.round(Math.sin(t / 400 + x / 20));
    ctx.fillStyle = c3; ctx.beginPath(); ctx.moveTo(x, y + 14); ctx.lineTo(x + 4 + sway, y); ctx.lineTo(x + 8, y + 14); ctx.fill();
    ctx.fillStyle = c1; ctx.beginPath(); ctx.moveTo(x + 1, y + 14); ctx.lineTo(x + 4 + sway, y + 3); ctx.lineTo(x + 7, y + 14); ctx.fill();
    rect(x + 3 + sway, y + 5, 1, 4, c2);
  }
  rect(0, y + 14, W, 6, c1);
}

// ------------------------------------------------------------- mapa (inicio)
// 15 x 7 casillas de 16 px: arboles, pasto, camino, flores y un estanque

function drawOverworld(y0, t, night) {
  const T = 16;
  for (let r = 0; r < 7; r++) for (let c = 0; c < 15; c++) {
    const x = c * T, y = y0 + r * T;
    const path = r === 4 || (c === 11 && r > 1);
    const pond = c >= 12 && r >= 5;
    if (r === 0) { tileGrass(x, y, c, r); tileTree(x, y); continue; }
    if (pond) { tileWater(x, y, t, c, r); continue; }
    if (path) { tilePath(x, y, c, r); continue; }
    tileGrass(x, y, c, r);
    const h = hash(c, r);
    if (h < 0.12) tileFlower(x, y, t, c);
    else if (h > 0.9 && r < 4) tileTree(x, y);
    else if (h > 0.8) tileTallGrass(x, y, t, c);
  }
  // casa a la derecha
  const hx = 176, hy = y0 + 18;
  rect(hx, hy + 10, 40, 26, '#e8d8b8');
  rect(hx - 4, hy, 48, 12, '#c84838');
  rect(hx - 4, hy + 10, 48, 2, '#983028');
  rect(hx + 16, hy + 22, 8, 14, '#785030');
  rect(hx + 4, hy + 16, 8, 7, night ? '#f8e070' : '#88c8f0');
  rect(hx + 28, hy + 16, 8, 7, night ? '#f8e070' : '#88c8f0');
  if (night) {
    ctx.fillStyle = 'rgba(16,24,72,0.45)';
    ctx.fillRect(0, y0, W, 7 * T);
    ctx.fillStyle = 'rgba(248,224,112,0.18)';
    ctx.beginPath(); ctx.arc(hx + 20, hy + 22, 26, 0, Math.PI * 2); ctx.fill();
  }
}

function tileGrass(x, y, c, r) {
  rect(x, y, 16, 16, '#88d070');
  ctx.fillStyle = '#70b858';
  for (let i = 0; i < 3; i++) {
    const px = x + Math.floor(hash(c * 3 + i, r) * 14), py = y + Math.floor(hash(c, r * 3 + i) * 13);
    ctx.fillRect(px, py, 1, 2); ctx.fillRect(px + 2, py + 1, 1, 2);
  }
}
function tilePath(x, y, c, r) {
  rect(x, y, 16, 16, '#e8d8a0');
  ctx.fillStyle = '#d0c088';
  for (let i = 0; i < 2; i++) ctx.fillRect(x + Math.floor(hash(c, r + i * 9) * 14), y + Math.floor(hash(r, c + i * 9) * 14), 2, 1);
}
function tileTree(x, y) {
  rect(x + 6, y + 10, 4, 6, '#785030');
  ctx.fillStyle = '#307830';
  ctx.beginPath(); ctx.arc(x + 8, y + 7, 7, 0, Math.PI * 2); ctx.fill();
  ctx.fillStyle = '#48a040';
  ctx.beginPath(); ctx.arc(x + 7, y + 6, 5, 0, Math.PI * 2); ctx.fill();
  rect(x + 5, y + 3, 2, 2, '#70c058');
}
function tileFlower(x, y, t, c) {
  const on = Math.floor(t / 500 + c) % 2;
  const col = c % 2 ? '#f85858' : '#f8d838';
  rect(x + 4, y + 5 + on, 3, 3, col); rect(x + 5, y + 6 + on, 1, 1, '#f8f8f8');
  rect(x + 10, y + 10 - on, 3, 3, col); rect(x + 11, y + 11 - on, 1, 1, '#f8f8f8');
}
function tileTallGrass(x, y, t, c) {
  const s = Math.round(Math.sin(t / 400 + c));
  ctx.fillStyle = '#40a040';
  for (let i = 0; i < 4; i++) { ctx.beginPath(); ctx.moveTo(x + i * 4, y + 15); ctx.lineTo(x + i * 4 + 2 + s, y + 5); ctx.lineTo(x + i * 4 + 4, y + 15); ctx.fill(); }
}
function tileWater(x, y, t, c, r) {
  rect(x, y, 16, 16, '#58a0f0');
  const o = Math.floor(t / 300 + c + r) % 4;
  rect(x + 2 + o, y + 4, 5, 1, '#a8d8f8');
  rect(x + 8 - o, y + 11, 5, 1, '#a8d8f8');
}

// --------------------------------------------- transicion al combate ------

function battleWipe(t0, t) {
  const p = (t - t0) / 700;
  if (p >= 1) return false;
  ctx.fillStyle = '#101010';
  const rows = 10, rh = H / rows;
  for (let i = 0; i < rows; i++) {
    const k = Math.max(0, Math.min(1, p * 2 - (i % 2) * 0.3));
    const w = W * (1 - k);
    if (i % 2) ctx.fillRect(W - w, i * rh, w, rh + 1);
    else ctx.fillRect(0, i * rh, w, rh + 1);
  }
  return true;
}

// ---------------------------------------------------- sprites de entrenadores
// Sprites de FireRed/LeafGreen del repositorio de Pokemon Showdown
// (r1/tools/fetch_assets.py). Mismo orden que TRAINER_CLASSES, GYMS y ELITE.
const TRAINER_KEYS = ['youngster', 'bugcatcher', 'lass', 'hiker', 'fisherman', 'swimmerm', 'birdkeeper', 'supernerd',
  'teamrocketgruntm', 'engineer', 'psychic', 'blackbelt', 'channeler', 'gentleman', 'beauty', 'acetrainer'];
const GYM_KEYS = ['brock', 'misty', 'ltsurge', 'erika', 'koga', 'sabrina', 'blaine', 'giovanni'];
const ELITE_KEYS = ['lorelei', 'bruno', 'agatha', 'lance', 'blue-champion'];
const trainerImgs = {}, trainerFeet = {};

function trainerImage(key) {
  if (!key) return null;
  if (!trainerImgs[key]) {
    const img = new Image();
    img.onload = () => {
      // donde estan los pies (los sprites traen margen abajo)
      const c = document.createElement('canvas');
      c.width = img.width; c.height = img.height;
      const g = c.getContext('2d');
      g.drawImage(img, 0, 0);
      const d = g.getImageData(0, 0, img.width, img.height).data;
      let bottom = img.height - 1;
      outer: for (let y = img.height - 1; y >= 0; y--) for (let x = 0; x < img.width; x++) if (d[(y * img.width + x) * 4 + 3] > 40) { bottom = y; break outer; }
      trainerFeet[key] = bottom;
    };
    img.src = `assets/trainers/${key}.png`;
    trainerImgs[key] = img;
  }
  const img = trainerImgs[key];
  return img.complete && img.naturalWidth ? img : null;
}

// entrenador con los pies en (x, y); devuelve false si su sprite no esta
function drawTrainer(key, x, y, opts = {}) {
  const img = trainerImage(key);
  if (!img) return false;
  const s = opts.scale || 1;
  const feet = trainerFeet[key] !== undefined ? trainerFeet[key] : img.height - 1;
  ctx.save();
  if (opts.alpha !== undefined) ctx.globalAlpha = opts.alpha;
  ctx.drawImage(img, Math.round(x - img.width * s / 2), Math.round(y - (feet + 1) * s), img.width * s, img.height * s);
  ctx.restore();
  return true;
}

// sprite del rival de un combate o encuentro (null si es salvaje)
function trainerKeyFor(v) {
  if (v.kind === 'rival') return 'blue';
  if (v.tclass !== undefined) return TRAINER_KEYS[v.tclass];
  if (v.tgym !== undefined) return GYM_KEYS[v.tgym];
  if (v.telite !== undefined) return ELITE_KEYS[v.telite];
  return null;
}
