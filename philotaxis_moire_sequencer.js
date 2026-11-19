// Phyllotaxis moiré sequencer — v8ui (Max 9+)
//
// The phyllotaxis point pattern of sousastep.quest's background shader
// (public/shaders/sousavfx-voronoi*.frag + public/js/vfx.js), drawn as
// individual points instead of voronoi cells. Each point is coloured exactly
// as the shader colours its cell: the ring/division mask evaluated at the
// site, tinted by the OKLCH synth palette.
//
// A line runs from the centre to the edge and rotates at `rpm` revolutions
// per minute (positive = clockwise; it starts pointing straight up). Every
// point it sweeps over plays a MIDI note:
//   pitch     chromatic, by the point's distance from the centre: noteMin at
//             the centre, noteMax at the rim (swap them to invert)
//   velocity  the brightness (HSB value = max(r, g, b)) of the point's colour
//   length    `duration` ms, the same for every note
// Notes softer than `gate` are skipped (0 = every point plays).
//
// Outlets:
//   0  list: pitch velocity duration   -> [makenote]
//   1  int:  MIDI channel              -> [noteout]'s right inlet
//
// Every parameter is a message (`rpm 12`, `hue 300`, ...), an attribute saved
// with the patcher, and part of the pattr state (getvalueof/setvalueof).
// Other messages: start, stop, reset, defaults, panel 0/1, bang.
// In the panel: drag a slider, click a toggle, double-click to reset.

autowatch = 1;
inlets = 1;
outlets = 2;
setinletassist(0, "parameter messages, start, stop, reset, defaults, panel 0/1");
setoutletassist(0, "list: pitch velocity duration (to makenote)");
setoutletassist(1, "MIDI channel (to noteout right inlet)");

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

// ---------------------------------------------------------------------------
// Parameters. The pattern/spiral/color ranges and defaults match the links
// page's shader panel (_layouts/default.html).
// ---------------------------------------------------------------------------

const ROWS = [
  { section: "sequencer" },
  { name: "play", label: "play", type: "toggle", def: 1 },
  { name: "rpm", label: "line rpm", min: -120, max: 120, step: 0.1, def: 6 },
  { name: "noteMin", label: "note min", min: 0, max: 127, step: 1, def: 36, note: true },
  { name: "noteMax", label: "note max", min: 0, max: 127, step: 1, def: 84, note: true },
  { name: "duration", label: "note length ms", min: 1, max: 4000, step: 1, def: 150, log: true },
  { name: "channel", label: "midi channel", min: 1, max: 16, step: 1, def: 1 },
  { name: "gate", label: "velocity gate", min: 0, max: 127, step: 1, def: 0 },
  { section: "pattern" },
  { name: "speed", label: "speed", min: -10, max: 10, step: 0.1, def: 0.7 },
  { name: "divisions", label: "divisions", min: 1, max: 233, step: 0.5, def: 56 },
  { name: "divLock", label: "lock divisions (off = sweep)", type: "toggle", def: 1 },
  { name: "maskType", label: "index-from-centre mask", type: "toggle", def: 0 },
  { section: "spiral" },
  { name: "points", label: "points", min: 2, max: 9001, step: 1, def: 201, log: true },
  { name: "discriminant", label: "φ discriminant", min: 1, max: 9, step: 0.001, def: 5 },
  { section: "color" },
  { swatch: true },
  { name: "hue", label: "hue", min: 0, max: 360, step: 1, def: 200 },
  { name: "hueSpan", label: "hue span", min: 0, max: 360, step: 1, def: 60 },
  { name: "lightness", label: "lightness", min: 0, max: 1, step: 0.01, def: 0.66 },
  { name: "lightSwing", label: "light swing", min: 0, max: 1, step: 0.01, def: 0.4 },
  { name: "chroma", label: "chroma", min: 0, max: 0.4, step: 0.005, def: 0.11 }
];

const PARAMS = ROWS.filter((r) => r.name);
const SPEC = {};
const P = {};
for (const p of PARAMS) {
  if (p.type === "toggle") { p.min = 0; p.max = 1; p.step = 1; }
  SPEC[p.name] = p;
  P[p.name] = p.def;
}

const NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
function noteName(n) {
  return NOTE_NAMES[n % 12] + (Math.floor(n / 12) - 1);
}

function decimalsFor(p) {
  if (p.log || p.step >= 1) return 0;
  return Math.max(0, Math.min(3, Math.ceil(-Math.log10(p.step) - 1e-9)));
}

function clampParam(p, v) {
  v = Number(v);
  if (!isFinite(v)) return P[p.name];
  if (p.type === "toggle") return v ? 1 : 0;
  v = Math.max(p.min, Math.min(p.max, v));
  if (p.step >= 1) return Math.round(v);
  const d = decimalsFor(p);
  return Number((Math.round(v / p.step) * p.step).toFixed(d));
}

// slider position 0..1 <-> value (log sliders as on the web panel)
function toNorm(p, v) {
  if (p.log) return Math.log(v / p.min) / Math.log(p.max / p.min);
  return (v - p.min) / (p.max - p.min);
}
function fromNorm(p, u) {
  u = Math.max(0, Math.min(1, u));
  if (p.log) return p.min * Math.pow(p.max / p.min, u);
  return p.min + u * (p.max - p.min);
}

// fromUI: the change came from the panel (moving divisions claims its lock,
// like the web panel)
function setParam(name, value, fromUI) {
  const p = SPEC[name];
  if (!p) return;
  const v = clampParam(p, value);
  if (name === "divLock" && v === 1 && P.divLock === 0) {
    // lock onto the value the pattern is showing right now
    P.divisions = clampParam(SPEC.divisions, divisionsS.value);
  }
  if (name === "divisions" && fromUI) P.divLock = 1;
  const changed = P[name] !== v;
  P[name] = v;
  if (name === "channel" && changed) outlet(1, v);
  if (name === "play" && v === 1 && changed) lastTime = Date.now();
  if (changed) mgraphics.redraw();
}

// ---------------------------------------------------------------------------
// Ports from vfx.js: smoothing, the unlocked divisions sweep, the OKLCH
// palette.
// ---------------------------------------------------------------------------

const ROT_RATE = -0.16;
const PALETTE_RATE = 0.008;
const DIVISION_SWEEP_RATE = 0.02536;
const SWEEP_MIN = 1;
const SWEEP_MAX = 233;
const SWEEP_SHAPE = 0.99;
const SWEEP_MID = (SWEEP_MIN + SWEEP_MAX) / 2;
const SWEEP_AMP = (SWEEP_MAX - SWEEP_MIN) / 2;
const SWEEP_NORM = Math.asin(SWEEP_SHAPE);
const SMOOTH_SECONDS = 0.3;
const STOP_COUNT = 7;
const ACHROMATIC_CHROMA = 0.002;
const MAX_POINTS = 9001;
const BASE = [21 / 255, 27 / 255, 38 / 255]; // #151b26, the shader's base colour

function clamp01(x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
function fract(x) { return x - Math.floor(x); }
function wrapTurnsSigned(dh) { return dh - Math.floor(dh + 0.5); }

function sweepDivisions(x) {
  return SWEEP_MID - SWEEP_AMP * Math.asin(SWEEP_SHAPE * Math.sin(x)) / SWEEP_NORM;
}
function sweepPhaseAt(v, rising) {
  const u = Math.max(-1, Math.min(1, (SWEEP_MID - v) / SWEEP_AMP));
  const x = Math.asin(Math.sin(u * SWEEP_NORM) / SWEEP_SHAPE);
  return rising ? Math.PI - x : x;
}

// critically damped smoothing (Game Programming Gems 4), as in palette_blend.h
function smoother(value) { return { value: value, velocity: 0 }; }
function smoothTo(s, target, omega, dt, decay, epsilon) {
  const change = s.value - target;
  const temp = (s.velocity + omega * change) * dt;
  s.velocity = (s.velocity - omega * temp) * decay;
  s.value = target + (change + temp) * decay;
  if (Math.abs(s.value - target) < epsilon) {
    s.value = target;
    s.velocity = 0;
  }
}

function mixOKLCH(a, b, t) {
  let ah = a.h;
  let bh = b.h;
  if (a.C < ACHROMATIC_CHROMA) ah = bh;
  else if (b.C < ACHROMATIC_CHROMA) bh = ah;
  const dh = wrapTurnsSigned(bh - ah);
  return { L: a.L + (b.L - a.L) * t, C: a.C + (b.C - a.C) * t, h: fract(ah + dh * t) };
}

function synthStops() {
  const stops = [];
  for (let i = 0; i < STOP_COUNT; i++) {
    const t = i / (STOP_COUNT - 1);
    stops.push({
      pos: t,
      L: clamp01(P.lightness + P.lightSwing * 0.5 * Math.sin(2 * Math.PI * t)),
      C: P.chroma,
      h: fract((P.hue + P.hueSpan * t) / 360)
    });
  }
  return stops;
}

// OKLCH -> sRGB 0..1 (quantised to 8 bits like the shader's palette texture),
// gamut-mapped toward the grey of the same lightness as in oklch.h
function oklchToSRGB(c) {
  const L = clamp01(c.L);
  const a = c.C * Math.cos(2 * Math.PI * c.h);
  const b = c.C * Math.sin(2 * Math.PI * c.h);
  const l_ = L + 0.3963377774 * a + 0.2158037573 * b;
  const m_ = L - 0.1055613458 * a - 0.0638541728 * b;
  const s_ = L - 0.0894841775 * a - 1.2914855480 * b;
  const l = l_ * l_ * l_, m = m_ * m_ * m_, s = s_ * s_ * s_;
  const rgb = [
    +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
    -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
    -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s
  ];
  const grey = L * L * L;
  let scale = 1;
  for (let i = 0; i < 3; i++) {
    if (rgb[i] > 1) scale = Math.min(scale, (1 - grey) / (rgb[i] - grey));
    else if (rgb[i] < 0) scale = Math.min(scale, grey / (grey - rgb[i]));
  }
  return rgb.map((v) => {
    v = clamp01(grey + scale * (v - grey));
    v = v <= 0.0031308 ? 12.92 * v : 1.055 * Math.pow(v, 1 / 2.4) - 0.055;
    return Math.round(v * 255) / 255;
  });
}

// the 16 palette entries (FastLED-style gradient sampling of the stops)
let palette = [];
let paletteKey = "";
function updatePalette() {
  const key = [P.hue, P.hueSpan, P.lightness, P.lightSwing, P.chroma].join();
  if (key === paletteKey) return;
  paletteKey = key;
  const stops = synthStops();
  palette = [];
  for (let i = 0; i < 16; i++) {
    const pos = i / 15;
    let k = 0;
    while (k < stops.length && stops[k].pos < pos) k++;
    let c;
    if (k === 0) c = stops[0];
    else if (k === stops.length) c = stops[stops.length - 1];
    else {
      const a = stops[k - 1], b = stops[k];
      c = mixOKLCH(a, b, (pos - a.pos) / (b.pos - a.pos));
    }
    palette.push(oklchToSRGB(c));
  }
}

// like the shader's LINEAR + REPEAT palette lookup: entry i sits at i / 16
function samplePalette(pos) {
  const x = fract(pos) * 16;
  const i = Math.floor(x) % 16;
  const j = (i + 1) % 16;
  const f = x - Math.floor(x);
  const a = palette[i], b = palette[j];
  return [a[0] + (b[0] - a[0]) * f, a[1] + (b[1] - a[1]) * f, a[2] + (b[2] - a[2]) * f];
}

// ---------------------------------------------------------------------------
// The pattern: the shader's pass 2, evaluated at each site.
// ---------------------------------------------------------------------------

function smootherstep(x) {
  x = clamp01(x);
  return x * x * x * (x * (x * 6 - 15) + 10);
}
function smoothstep(e0, e1, x) {
  const t = clamp01((x - e0) / (e1 - e0));
  return t * t * (3 - 2 * t);
}
function divisionMask(p) {
  const width = 0.498;
  const peak = width * 0.5;
  const tri = clamp01(p < peak ? p / peak : 1 - (p - peak) / (width - peak));
  return smootherstep(tri);
}

const siteRad = new Float64Array(MAX_POINTS);
const siteAng = new Float64Array(MAX_POINTS); // turns, counter-clockwise from +x
const siteFlash = new Float64Array(MAX_POINTS); // time of the last trigger (ms)
let siteCount = 0;
let siteD = -1;

// buildCoordinateMaps(): site n at n * golden angle, radius sqrt(n / (count - 1))
function updateSites(discriminant) {
  const count = Math.max(2, Math.min(MAX_POINTS, Math.round(P.points)));
  if (count === siteCount && discriminant === siteD) return;
  const g = 1 - 2 / (1 + Math.sqrt(Math.max(discriminant, 0)));
  for (let n = 0; n < count; n++) {
    siteRad[n] = Math.sqrt(n / (count - 1));
    siteAng[n] = fract(n * g);
  }
  if (count !== siteCount) siteFlash.fill(0);
  siteCount = count;
  siteD = discriminant;
}

// the colour the shader gives site n's cell, at the site (halo = 1 there and
// the cell silhouette is saturated), as sRGB 0..1
const colorOut = [0, 0, 0];
function siteColor(n) {
  const totalPhase = (P.maskType ? n / 200 : siteAng[n]) * divisionsS.value;
  const phase = fract(totalPhase + rotPhase);
  const glow = divisionMask(phase) * (1 - smoothstep(0.7, 1.15, siteRad[n]));
  const lit = samplePalette(totalPhase + palPhase);
  for (let i = 0; i < 3; i++) {
    colorOut[i] = clamp01(BASE[i] + (lit[i] - BASE[i]) * glow * 0.42 + lit[i] * glow * 0.2);
  }
  return colorOut;
}

function brightness(c) {
  return Math.max(c[0], c[1], c[2]);
}

// ---------------------------------------------------------------------------
// Animation + sequencing, run from a Task.
// ---------------------------------------------------------------------------

let rotPhase = 0;
let palPhase = 0;
let sweepPhase = 0;
let sweepLocked = true;
const speedS = smoother(P.speed);
const divisionsS = smoother(P.divisions);
const discriminantS = smoother(P.discriminant);
let linePhase = 0.25; // turns, counter-clockwise from +x: 0.25 = straight up
let lastTime = Date.now();
let lastPaint = 0;
let lastNote = null;
let panelVisible = true;

const MAX_NOTES_PER_TICK = 32;
const PAINT_MS = 1000 / 60;
const FLASH_MS = 220;

function stepPattern(dt) {
  const omega = 2 / SMOOTH_SECONDS;
  const x = omega * dt;
  const decay = 1 / (1 + x + 0.48 * x * x + 0.235 * x * x * x);

  smoothTo(speedS, P.speed, omega, dt, decay, 1e-4);
  const t = speedS.value * dt;
  rotPhase = fract(rotPhase + ROT_RATE * t);
  palPhase = fract(palPhase + PALETTE_RATE * t);

  if (sweepLocked && !P.divLock) {
    sweepPhase = sweepPhaseAt(divisionsS.value, Math.cos(sweepPhase) < 0);
  }
  sweepLocked = !!P.divLock;
  if (!sweepLocked) sweepPhase = (sweepPhase + DIVISION_SWEEP_RATE * t) % (2 * Math.PI);
  smoothTo(divisionsS, sweepLocked ? P.divisions : sweepDivisions(sweepPhase), omega, dt, decay, 1e-4);

  smoothTo(discriminantS, P.discriminant, omega, dt, decay, 1e-5);
  updateSites(discriminantS.value);
  updatePalette();
}

// plays every site the line swept over, going from `from` to `to` (turns)
function sweepLine(from, to, now) {
  const delta = Math.abs(to - from);
  if (delta <= 0) return;
  const clockwise = to < from;
  const hits = [];
  for (let n = 0; n < siteCount; n++) {
    // (from, to]: d is how far past `from` the site sits, in the line's direction
    const d = clockwise ? fract(from - siteAng[n]) : fract(siteAng[n] - from);
    if (!(delta >= 1 || (d > 0 && d <= delta))) continue;
    siteFlash[n] = now;
    const vel = Math.round(brightness(siteColor(n)) * 127);
    if (vel < 1 || vel < P.gate) continue;
    const pitch = Math.round(P.noteMin + (P.noteMax - P.noteMin) * siteRad[n]);
    hits.push([pitch, vel]);
  }
  if (!hits.length) return;
  // one note per pitch per tick (the loudest), capped
  const byPitch = new Map();
  for (const h of hits) {
    const prev = byPitch.get(h[0]);
    if (!prev || h[1] > prev[1]) byPitch.set(h[0], h);
  }
  const notes = Array.from(byPitch.values()).sort((a, b) => b[1] - a[1]).slice(0, MAX_NOTES_PER_TICK);
  for (const [pitch, vel] of notes) {
    outlet(0, pitch, vel, P.duration);
    lastNote = { pitch: pitch, vel: vel, time: now };
  }
}

function tick() {
  const now = Date.now();
  const dt = Math.min((now - lastTime) / 1000, 0.1);
  lastTime = now;
  stepPattern(dt);
  if (P.play) {
    const from = linePhase;
    const to = linePhase - (P.rpm / 60) * dt; // positive rpm = clockwise
    sweepLine(from, to, now);
    linePhase = fract(to);
  }
  if (now - lastPaint >= PAINT_MS) {
    lastPaint = now;
    mgraphics.redraw();
  }
}

const task = new Task(tick, this);
task.interval = 5;
task.repeat();

function notifydeleted() {
  task.cancel();
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

const PANEL_W = 220;
const ROW_H = { slider: 24, toggle: 18, section: 20, swatch: 14 };
const COL = {
  bg: [0.043, 0.055, 0.078, 1],
  panel: [0.075, 0.09, 0.122, 1],
  text: [0.86, 0.88, 0.92, 1],
  dim: [0.55, 0.59, 0.66, 1],
  track: [0.16, 0.19, 0.25, 1],
  fill: [0.47, 0.71, 0.86, 1],
  line: [1, 1, 1, 0.85]
};

function setColor(c, alpha) {
  mgraphics.set_source_rgba(c[0], c[1], c[2], alpha === undefined ? (c[3] === undefined ? 1 : c[3]) : alpha);
}

// panel rows laid out top to bottom, squeezed to fit the box height
function layout(w, h) {
  if (!panelVisible || w < PANEL_W + 120) return { x: w, w: 0, rows: [] };
  const x = w - PANEL_W;
  let total = 8;
  for (const r of ROWS) total += rowHeight(r);
  const k = Math.min(1, (h - 8) / total);
  const rows = [];
  let y = 6;
  for (const r of ROWS) {
    const rh = rowHeight(r) * k;
    rows.push({ row: r, x: x + 10, y: y, w: PANEL_W - 20, h: rh });
    y += rh;
  }
  return { x: x, w: PANEL_W, rows: rows };
}

function rowHeight(r) {
  if (r.section) return ROW_H.section;
  if (r.swatch) return ROW_H.swatch;
  if (r.type === "toggle") return ROW_H.toggle;
  return ROW_H.slider;
}

function displayValue(p) {
  let v = P[p.name];
  if (p.name === "divisions" && !P.divLock) v = divisionsS.value;
  if (p.note) return v + " " + noteName(v);
  return v.toFixed(decimalsFor(p));
}

function paint() {
  const size = mgraphics.size;
  const w = size[0], h = size[1];
  const L = layout(w, h);

  setColor(COL.bg);
  mgraphics.rectangle(0, 0, w, h);
  mgraphics.fill();

  drawPattern(L.x, h);
  if (L.w > 0) drawPanel(L, h);
}

function drawPattern(areaW, h) {
  const cx = areaW / 2, cy = h / 2;
  const R = Math.max(10, Math.min(areaW, h) / 2 - 14);
  const now = Date.now();

  // rim
  mgraphics.set_line_width(1);
  setColor(COL.track, 0.6);
  mgraphics.ellipse(cx - R, cy - R, 2 * R, 2 * R);
  mgraphics.stroke();

  // dot size from the spacing of equal-area cells
  const spacing = R * Math.sqrt(Math.PI / siteCount);
  const dr = Math.max(0.75, Math.min(10, spacing * 0.36));
  const square = dr < 1.6;

  for (let n = 0; n < siteCount; n++) {
    const a = 2 * Math.PI * siteAng[n];
    const r = R * siteRad[n];
    const x = cx + r * Math.cos(a);
    const y = cy - r * Math.sin(a);
    const c = siteColor(n);
    mgraphics.set_source_rgba(c[0], c[1], c[2], 1);
    if (square) mgraphics.rectangle(x - dr, y - dr, 2 * dr, 2 * dr);
    else mgraphics.ellipse(x - dr, y - dr, 2 * dr, 2 * dr);
    mgraphics.fill();
  }

  // trigger flashes
  mgraphics.set_line_width(Math.max(1, dr * 0.35));
  for (let n = 0; n < siteCount; n++) {
    const age = now - siteFlash[n];
    if (siteFlash[n] === 0 || age > FLASH_MS) continue;
    const f = 1 - age / FLASH_MS;
    const a = 2 * Math.PI * siteAng[n];
    const r = R * siteRad[n];
    const x = cx + r * Math.cos(a);
    const y = cy - r * Math.sin(a);
    const rr = dr + 1 + dr * 1.2 * (1 - f);
    mgraphics.set_source_rgba(1, 1, 1, 0.75 * f);
    mgraphics.ellipse(x - rr, y - rr, 2 * rr, 2 * rr);
    mgraphics.stroke();
  }

  // the sweep line
  const la = 2 * Math.PI * linePhase;
  mgraphics.set_line_width(1.5);
  setColor(COL.line, P.play ? 0.85 : 0.35);
  mgraphics.move_to(cx, cy);
  mgraphics.line_to(cx + (R + 6) * Math.cos(la), cy - (R + 6) * Math.sin(la));
  mgraphics.stroke();

  // status line
  mgraphics.select_font_face("Arial");
  mgraphics.set_font_size(10);
  setColor(COL.dim);
  let status = (P.play ? "▶ " : "■ ") + siteCount + " points · " + P.rpm.toFixed(1) + " rpm";
  if (lastNote && now - lastNote.time < 1500) {
    status += " · " + noteName(lastNote.pitch) + " v" + lastNote.vel;
  }
  mgraphics.move_to(8, h - 8);
  mgraphics.show_text(status);
}

function drawPanel(L, h) {
  setColor(COL.panel);
  mgraphics.rectangle(L.x, 0, L.w, h);
  mgraphics.fill();

  mgraphics.select_font_face("Arial");
  for (const cell of L.rows) {
    const r = cell.row;
    if (r.section) {
      mgraphics.set_font_size(9);
      setColor(COL.dim);
      mgraphics.move_to(cell.x, cell.y + cell.h - 6);
      mgraphics.show_text(r.section.toUpperCase());
    } else if (r.swatch) {
      const n = 32;
      const sw = cell.w / n;
      for (let i = 0; i < n; i++) {
        const c = samplePalette(i / n);
        mgraphics.set_source_rgba(c[0], c[1], c[2], 1);
        mgraphics.rectangle(cell.x + i * sw, cell.y + 2, sw + 0.5, cell.h - 5);
        mgraphics.fill();
      }
    } else if (r.type === "toggle") {
      const on = P[r.name] === 1;
      const bs = Math.min(11, cell.h - 5);
      const by = cell.y + (cell.h - bs) / 2;
      setColor(on ? COL.fill : COL.track);
      mgraphics.rectangle_rounded(cell.x, by, bs, bs, 3, 3);
      mgraphics.fill();
      mgraphics.set_font_size(10);
      setColor(on ? COL.text : COL.dim);
      mgraphics.move_to(cell.x + bs + 7, by + bs - 2);
      mgraphics.show_text(r.label);
    } else {
      const free = r.name === "divisions" && !P.divLock; // following the sweep
      mgraphics.set_font_size(10);
      setColor(free ? COL.dim : COL.text);
      const baseY = cell.y + cell.h * 0.5;
      mgraphics.move_to(cell.x, baseY);
      mgraphics.show_text(r.label);
      const txt = displayValue(r);
      const tw = mgraphics.text_measure(txt)[0];
      mgraphics.move_to(cell.x + cell.w - tw, baseY);
      mgraphics.show_text(txt);

      const ty = cell.y + cell.h * 0.62;
      const th = Math.max(3, cell.h * 0.18);
      setColor(COL.track);
      mgraphics.rectangle_rounded(cell.x, ty, cell.w, th, th, th);
      mgraphics.fill();
      const v = free ? divisionsS.value : P[r.name];
      const u = clamp01(toNorm(r, v));
      // bipolar sliders fill from zero
      const zero = r.min < 0 && r.max > 0 ? clamp01(toNorm(r, 0)) : 0;
      const x0 = cell.x + cell.w * Math.min(zero, u);
      const x1 = cell.x + cell.w * Math.max(zero, u);
      setColor(COL.fill, free ? 0.45 : 1);
      mgraphics.rectangle_rounded(x0, ty, Math.max(th, x1 - x0), th, th, th);
      mgraphics.fill();
    }
  }
}

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------

let dragging = null;

function hitRow(x, y) {
  const size = mgraphics.size;
  const L = layout(size[0], size[1]);
  if (x < L.x) return null;
  for (const cell of L.rows) {
    if (y >= cell.y && y < cell.y + cell.h && cell.row.name) return cell;
  }
  return null;
}

function setFromX(cell, x) {
  const p = cell.row;
  setParam(p.name, fromNorm(p, (x - cell.x) / cell.w), true);
}

function onclick(x, y, but, cmd, shift, capslock, option, ctrl) {
  const cell = hitRow(x, y);
  if (!cell) return;
  if (cell.row.type === "toggle") {
    setParam(cell.row.name, P[cell.row.name] ? 0 : 1, true);
    dragging = null;
  } else {
    dragging = cell;
    setFromX(cell, x);
  }
}
onclick.local = 1;

function ondrag(x, y, but) {
  if (!dragging) return;
  if (!but) {
    dragging = null;
    return;
  }
  setFromX(dragging, x);
}
ondrag.local = 1;

function ondblclick(x, y) {
  const cell = hitRow(x, y);
  if (!cell) return;
  dragging = null;
  setParam(cell.row.name, cell.row.def, true);
}
ondblclick.local = 1;

function onresize() {
  mgraphics.redraw();
}
onresize.local = 1;

// ---------------------------------------------------------------------------
// Messages, attributes, pattr
// ---------------------------------------------------------------------------

function start() { setParam("play", 1); }
function stop() { setParam("play", 0); }

function reset() {
  linePhase = 0.25;
  rotPhase = 0;
  palPhase = 0;
  sweepPhase = 0;
  siteFlash.fill(0);
  mgraphics.redraw();
}

function defaults() {
  for (const p of PARAMS) setParam(p.name, p.def);
}

function panel(v) {
  panelVisible = !!v;
  mgraphics.redraw();
}

function bang() {
  outlet(1, P.channel);
}

function loadbang() {
  outlet(1, P.channel);
}

// `<param> <value>` (also reached through the attributes below)
function anything() {
  if (SPEC[messagename]) setParam(messagename, arguments[0]);
  else post("philotaxis: unknown message " + messagename + "\n");
}

// attributes, stored with the patcher; getters/setters are generated globals
for (const p of PARAMS) {
  const g = "get_" + p.name, s = "set_" + p.name;
  globalThis[g] = () => P[p.name];
  globalThis[s] = (v) => setParam(p.name, v);
  try {
    declareattribute(p.name, g, s, 1);
  } catch (e) {
    post("philotaxis: could not declare attribute " + p.name + "\n");
  }
}

// pattr: every parameter value, in panel order
function getvalueof() {
  return PARAMS.map((p) => P[p.name]);
}

function setvalueof() {
  const vals = Array.prototype.slice.call(arguments);
  PARAMS.forEach((p, i) => {
    if (i < vals.length) setParam(p.name, vals[i]);
  });
}

updateSites(P.discriminant);
updatePalette();
mgraphics.redraw();
