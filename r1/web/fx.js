'use strict';
// Efectos de los ataques por tipo, dibujados por codigo (como los del juego:
// llamas, burbujas, rayos, hojas, rocas, anillos psiquicos...), el temblor de
// pantalla y la animacion de evolucion.

const T = { NORMAL: 0, FIGHTING: 1, FLYING: 2, POISON: 3, GROUND: 4, ROCK: 5, BUG: 6, GHOST: 7, FIRE: 8,
            WATER: 9, GRASS: 10, ELECTRIC: 11, PSYCHIC: 12, ICE: 13, DRAGON: 14 };
// categorias de movimiento que van sobre el propio atacante (suben stats, curan, protegen)
const SELF_CATS = new Set([5, 7, 8, 11]);  // MC_FIELD_EFFECT, MC_HEAL, MC_NET_GOOD_STATS, MC_WHOLE_FIELD_EFFECT

// centro de cada Pokemon en el combate (lado 0 = el tuyo)
const FX_POS = [{ x: 64, y: 142 }, { x: 176, y: 66 }];
const FX_DUR = 700;

class MoveFx {
  constructor(ev, t) {
    this.type = ev.t;
    this.t0 = t;
    this.status = !ev.p;
    this.self = this.status && SELF_CATS.has(ev.c);
    this.from = FX_POS[ev.s];
    this.to = FX_POS[this.self ? ev.s : 1 - ev.s];
    this.seed = Math.floor(t) % 997;
  }
  // devuelve false cuando termina
  draw(t) {
    const p = (t - this.t0) / FX_DUR;
    if (p >= 1) return false;
    if (p < 0) return true;
    ctx.save();
    if (this.status) this.drawStatus(p, t);
    else this.drawHit(p, t);
    ctx.restore();
    return true;
  }
  r(i, k = 0) { return hash(this.seed + i, k); }
  // punto en el camino del atacante al rival (con arco opcional)
  path(q, arc = 0) {
    return { x: this.from.x + (this.to.x - this.from.x) * q, y: this.from.y + (this.to.y - this.from.y) * q - Math.sin(q * Math.PI) * arc };
  }
  drawHit(p, t) {
    const to = this.to;
    switch (this.type) {
      case T.FIRE: {
        for (let i = 0; i < 9; i++) {
          const q = Math.min(1, p * 1.6 - i * 0.06);
          if (q <= 0) continue;
          const pt = this.path(q, 14);
          const s = 3 + this.r(i) * 4 + (q >= 1 ? (p - 0.6) * 20 : 0);
          ctx.fillStyle = ['#f8d030', '#f89030', '#f85030'][(i + Math.floor(t / 60)) % 3];
          ctx.globalAlpha = q >= 1 ? Math.max(0, 1 - (p - 0.6) * 2.5) : 1;
          ctx.beginPath(); ctx.arc(pt.x + (this.r(i, 1) - 0.5) * 10, pt.y + (this.r(i, 2) - 0.5) * 10, s, 0, Math.PI * 2); ctx.fill();
        }
        break;
      }
      case T.WATER: {
        for (let i = 0; i < 10; i++) {
          const q = Math.min(1, p * 1.7 - i * 0.05);
          if (q <= 0) continue;
          const pt = this.path(q, 26);
          ctx.fillStyle = i % 2 ? '#78b8f8' : '#3878e0';
          ctx.strokeStyle = '#e8f4ff';
          ctx.beginPath(); ctx.arc(pt.x, pt.y, 3 + (i % 3), 0, Math.PI * 2); ctx.fill(); ctx.stroke();
        }
        if (p > 0.6) splash(to, (p - 0.6) / 0.4, '#a8d8f8');
        break;
      }
      case T.GRASS: {
        for (let i = 0; i < 8; i++) {
          const q = Math.min(1, p * 1.5 - i * 0.07);
          if (q <= 0) continue;
          const pt = this.path(q, 10 + i * 2);
          ctx.save();
          ctx.translate(pt.x, pt.y + (i % 3 - 1) * 6);
          ctx.rotate(t / 80 + i);
          ctx.fillStyle = i % 2 ? '#58c048' : '#88d860';
          ctx.beginPath(); ctx.ellipse(0, 0, 5, 2.5, 0, 0, Math.PI * 2); ctx.fill();
          ctx.restore();
        }
        break;
      }
      case T.ELECTRIC: {
        if (p < 0.7) {
          ctx.strokeStyle = '#f8f060';
          ctx.lineWidth = 3;
          ctx.beginPath();
          let x = to.x + (this.r(1) - 0.5) * 20, y = -4;
          ctx.moveTo(x, y);
          const steps = 6;
          for (let i = 1; i <= steps; i++) {
            x = to.x + (hash(this.seed + i, Math.floor(t / 50)) - 0.5) * 24;
            y = (to.y + 10) * i / steps;
            ctx.lineTo(x, y);
          }
          ctx.stroke();
          ctx.strokeStyle = '#ffffff'; ctx.lineWidth = 1; ctx.stroke();
          if (Math.floor(t / 60) % 2) { ctx.fillStyle = 'rgba(248,240,120,0.25)'; ctx.fillRect(0, 0, W, 176); }
        }
        sparks(to, p, '#f8f060');
        break;
      }
      case T.ICE: {
        for (let i = 0; i < 7; i++) {
          const q = Math.min(1, p * 1.6 - i * 0.07);
          if (q <= 0) continue;
          const pt = this.path(q, 6);
          ctx.save(); ctx.translate(pt.x, pt.y + (i % 3 - 1) * 5); ctx.rotate(Math.atan2(this.to.y - this.from.y, this.to.x - this.from.x));
          ctx.fillStyle = i % 2 ? '#c8f0f8' : '#88d0f0';
          ctx.beginPath(); ctx.moveTo(7, 0); ctx.lineTo(-5, -3); ctx.lineTo(-5, 3); ctx.fill();
          ctx.restore();
        }
        if (p > 0.6) sparks(to, (p - 0.6) / 0.4, '#e8f8ff');
        break;
      }
      case T.PSYCHIC: {
        for (let i = 0; i < 3; i++) {
          const q = (p * 1.4 - i * 0.18);
          if (q <= 0 || q >= 1) continue;
          ctx.strokeStyle = `rgba(248,88,160,${1 - q})`;
          ctx.lineWidth = 2;
          ctx.beginPath(); ctx.arc(to.x, to.y, 6 + q * 30, 0, Math.PI * 2); ctx.stroke();
        }
        ctx.fillStyle = `rgba(248,120,200,${0.15 * Math.sin(p * Math.PI)})`;
        ctx.fillRect(0, 0, W, 176);
        break;
      }
      case T.GHOST: {
        ctx.fillStyle = `rgba(40,16,64,${0.45 * Math.sin(p * Math.PI)})`;
        ctx.fillRect(0, 0, W, 176);
        for (let i = 0; i < 2; i++) {
          const q = p * 1.3 - i * 0.25;
          if (q <= 0 || q >= 1) continue;
          ctx.strokeStyle = `rgba(160,96,200,${1 - q})`;
          ctx.lineWidth = 3;
          ctx.beginPath(); ctx.ellipse(to.x, to.y, 8 + q * 26, 4 + q * 12, 0, 0, Math.PI * 2); ctx.stroke();
        }
        break;
      }
      case T.POISON: {
        for (let i = 0; i < 8; i++) {
          const q = (p * 1.5 - this.r(i) * 0.5);
          if (q <= 0 || q >= 1) continue;
          ctx.fillStyle = i % 2 ? '#a040c0' : '#c878e0';
          ctx.globalAlpha = 1 - q;
          ctx.beginPath(); ctx.arc(to.x + (this.r(i, 1) - 0.5) * 30, to.y + 12 - q * 34, 3 + (i % 3), 0, Math.PI * 2); ctx.fill();
        }
        break;
      }
      case T.GROUND:
      case T.ROCK: {
        for (let i = 0; i < 6; i++) {
          const q = Math.min(1, p * 1.6 - i * 0.08);
          if (q <= 0) continue;
          const x = to.x + (this.r(i) - 0.5) * 36, y = -10 + (to.y + 8) * q;
          ctx.fillStyle = this.type === T.ROCK ? (i % 2 ? '#a89060' : '#806848') : (i % 2 ? '#c0a060' : '#987840');
          ctx.beginPath(); ctx.moveTo(x - 5, y); ctx.lineTo(x, y - 6); ctx.lineTo(x + 6, y - 1); ctx.lineTo(x + 3, y + 5); ctx.lineTo(x - 4, y + 4); ctx.fill();
        }
        break;
      }
      case T.FLYING: {
        ctx.strokeStyle = 'rgba(255,255,255,0.9)';
        ctx.lineWidth = 2;
        for (let i = 0; i < 3; i++) {
          const q = p * 1.4 - i * 0.15;
          if (q <= 0 || q >= 1) continue;
          const x = to.x - 40 + q * 80;
          ctx.beginPath(); ctx.arc(x, to.y + (i - 1) * 10, 14, -0.9, 0.9); ctx.stroke();
        }
        break;
      }
      case T.BUG: {
        for (let i = 0; i < 8; i++) {
          const q = Math.min(1, p * 1.8 - i * 0.08);
          if (q <= 0 || q >= 1) continue;
          const pt = this.path(q, 4);
          ctx.fillStyle = '#a8c830';
          ctx.fillRect(Math.round(pt.x - 2), Math.round(pt.y - 2 + (i % 2) * 4), 4, 4);
        }
        if (p > 0.55) impact(to, (p - 0.55) / 0.45, '#d8f070');
        break;
      }
      case T.DRAGON: {
        for (let i = 0; i < 10; i++) {
          const q = Math.min(1, p * 1.6 - i * 0.05);
          if (q <= 0) continue;
          const pt = this.path(q, 20);
          const a = t / 70 + i;
          ctx.fillStyle = i % 2 ? '#7038f8' : '#58a0f8';
          ctx.beginPath(); ctx.arc(pt.x + Math.cos(a) * 8, pt.y + Math.sin(a) * 8, 3, 0, Math.PI * 2); ctx.fill();
        }
        break;
      }
      default:  // Normal y Lucha: golpe con estrellas en el rival
        if (p > 0.3) impact(to, (p - 0.3) / 0.7, this.type === T.FIGHTING ? '#f89030' : '#f8f8f8');
    }
  }
  // movimientos de estado: brillo que sube (propio) o anillos de color (rival)
  drawStatus(p) {
    const to = this.to;
    if (this.self) {
      for (let i = 0; i < 6; i++) {
        const q = (p * 1.3 - this.r(i) * 0.4);
        if (q <= 0 || q >= 1) continue;
        const x = to.x + (this.r(i, 1) - 0.5) * 36, y = to.y + 16 - q * 40;
        ctx.globalAlpha = 1 - q;
        ctx.fillStyle = '#88c8f8';
        ctx.beginPath(); ctx.moveTo(x, y - 5); ctx.lineTo(x + 4, y + 1); ctx.lineTo(x - 4, y + 1); ctx.fill();
      }
    } else {
      const c = TYPE_COLORS[this.type] || '#f8f8f8';
      for (let i = 0; i < 2; i++) {
        const q = p * 1.3 - i * 0.25;
        if (q <= 0 || q >= 1) continue;
        ctx.globalAlpha = 1 - q;
        ctx.strokeStyle = c;
        ctx.lineWidth = 2;
        ctx.beginPath(); ctx.arc(to.x, to.y, 30 - q * 22, 0, Math.PI * 2); ctx.stroke();
      }
    }
  }
}

function impact(at, q, color) {
  ctx.globalAlpha = Math.max(0, 1 - q);
  ctx.fillStyle = color;
  ctx.strokeStyle = '#383840';
  const r = 6 + q * 12;
  ctx.beginPath();
  for (let i = 0; i < 16; i++) {
    const a = i / 16 * Math.PI * 2, rr = i % 2 ? r * 0.45 : r;
    ctx.lineTo(at.x + Math.cos(a) * rr, at.y + Math.sin(a) * rr);
  }
  ctx.closePath(); ctx.fill(); ctx.stroke();
  ctx.globalAlpha = 1;
}

function sparks(at, q, color) {
  ctx.globalAlpha = Math.max(0, 1 - q);
  ctx.fillStyle = color;
  for (let i = 0; i < 8; i++) {
    const a = i / 8 * Math.PI * 2, d = 6 + q * 22;
    ctx.fillRect(Math.round(at.x + Math.cos(a) * d) - 1, Math.round(at.y + Math.sin(a) * d) - 1, 3, 3);
  }
  ctx.globalAlpha = 1;
}

function splash(at, q, color) {
  ctx.globalAlpha = Math.max(0, 1 - q);
  ctx.fillStyle = color;
  for (let i = 0; i < 7; i++) {
    const a = Math.PI + i / 6 * Math.PI, d = 4 + q * 20;
    ctx.beginPath(); ctx.arc(at.x + Math.cos(a) * d, at.y + 10 + Math.sin(a) * d * 0.8, 2.5, 0, Math.PI * 2); ctx.fill();
  }
  ctx.globalAlpha = 1;
}

// temblor de pantalla: amplitud que se apaga en `dur` ms
function shakeOffset(t0, t, dur = 320, amp = 4) {
  if (!t0 || t - t0 > dur) return 0;
  const k = 1 - (t - t0) / dur;
  return Math.round(Math.sin((t - t0) / 18) * amp * k);
}

// ------------------------------------------------------------- evolucion ----
// La silueta blanca alterna entre las dos formas cada vez mas rapido; volver
// (◀ o presion larga) la detiene, como el boton B en el juego.

class EvolutionScreen {
  constructor(p, onDone) {
    this.p = p;
    this.onDone = onDone;
    this.t0 = performance.now();
    this.phase = 'evolving';
    spriteImage(p.sp);
    spriteImage(p.to);
  }
  draw(t) {
    const dt = t - this.t0;
    rect(0, 0, W, H, '#101420');
    for (let i = 0; i < 30; i++) {  // destellos alrededor
      const a = i * 2.4 + dt / 900, d = 40 + ((dt / 8 + i * 37) % 80);
      rect(Math.round(W / 2 + Math.cos(a) * d), Math.round(118 + Math.sin(a) * d * 0.8), 2, 2, i % 3 ? '#f8f8f8' : '#f8e070');
    }
    const p = this.p;
    const DUR = 4200;
    if (this.phase === 'evolving') {
      const k = Math.min(1, dt / DUR);
      const period = 620 - 560 * k;
      const showNew = Math.floor(dt / period) % 2 === 1;
      const sp = showNew ? p.to : p.sp, hd = showNew ? p.toHd : p.hd;
      drawMon(sp, 'front', W / 2, 150, t, { scale: monScale(sp, hd || 10, 'front', 56), flash: dt < 500 ? dt / 500 : 1 });
      if (dt > DUR) { this.phase = 'flash'; this.flashT = t; }
    } else if (this.phase === 'flash') {
      const q = (t - this.flashT) / 500;
      drawMon(p.to, 'front', W / 2, 150, t, { scale: monScale(p.to, p.toHd || 10, 'front', 56), flash: Math.max(0, 1 - q) });
      rect(0, 0, W, H, `rgba(255,255,255,${Math.max(0, 1 - q)})`);
      if (q >= 1) { this.phase = 'done'; this.onDone(true); }
    } else {
      drawMon(this.phase === 'stopped' ? p.sp : p.to, 'front', W / 2, 150, t,
        { scale: monScale(this.phase === 'stopped' ? p.sp : p.to, (this.phase === 'stopped' ? p.hd : p.toHd) || 10, 'front', 56) });
    }
    const msg = this.phase === 'evolving' ? L('evolving', p.name) : this.phase === 'stopped' ? L('stopped', p.name)
              : this.phase === 'flash' ? '' : L('evolved', p.name, p.toName);
    if (msg) dialogBox(wrap(msg, 26).slice(0, 3), t, this.phase === 'done' || this.phase === 'stopped');
    if (this.phase === 'evolving') { backButton(); textS(L('stopEvo'), 24, 5, '#f8f8f8', 'left', '#182438'); }
  }
  input(ev) {
    if (this.phase === 'evolving' && (ev.k === 'back' || inBack(ev))) { this.phase = 'stopped'; this.onDone(false); return; }
    if ((this.phase === 'done' || this.phase === 'stopped') && (ev.k === 'ok' || ev.k === 'tap' || ev.k === 'back')) {
      pop();
      if (this.then) this.then();
    }
  }
}
