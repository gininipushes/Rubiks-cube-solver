// cubemodel.js - facelet-level cube model for the visualiser (no 3D code).
// Facelet order and orientation follow Kociemba's URFDLB layout, the same
// format the C++ solver reads.
export const FACES = "URFDLB";
export const SOLVED = FACES.split("").map((f) => f.repeat(9)).join("");

// Cubie position p (each coordinate -1, 0 or 1) and outward normal n of
// sticker i. x points right, y up, z towards the front face.
function stickerInfo(i) {
  const f = Math.floor(i / 9), k = i % 9, r = Math.floor(k / 3), c = k % 3;
  switch (f) {
    case 0: return { p: [c - 1, 1, r - 1], n: [0, 1, 0] };   // U, B edge at top
    case 1: return { p: [1, 1 - r, 1 - c], n: [1, 0, 0] };   // R
    case 2: return { p: [c - 1, 1 - r, 1], n: [0, 0, 1] };   // F
    case 3: return { p: [c - 1, -1, 1 - r], n: [0, -1, 0] }; // D, F edge at top
    case 4: return { p: [-1, 1 - r, c - 1], n: [-1, 0, 0] }; // L
    default: return { p: [1 - c, 1 - r, -1], n: [0, 0, -1] }; // B
  }
}
export const STICKERS = Array.from({ length: 54 }, (_, i) => stickerInfo(i));
// Outward axis of each face; a clockwise turn is -90 degrees about it.
export const AXES = [[0, 1, 0], [1, 0, 0], [0, 0, 1], [0, -1, 0], [-1, 0, 0], [0, 0, -1]];

const key = (p, n) => p.join() + "|" + n.join();
const INDEX = new Map(STICKERS.map((s, i) => [key(s.p, s.n), i]));
export const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
const cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
// Rotate v by -90 degrees about unit axis a (Rodrigues, cos = 0, sin = -1).
const rot = (a, v) => {
  const c = cross(a, v), d = dot(a, v);
  return [a[0] * d - c[0], a[1] * d - c[1], a[2] * d - c[2]];
};

// DEST[face][i] = where sticker i ends up after a clockwise quarter turn.
const DEST = AXES.map((a) =>
  STICKERS.map((s, i) => (dot(s.p, a) === 1 ? INDEX.get(key(rot(a, s.p), rot(a, s.n))) : i)));

// Moves are numbered face * 3 + (power - 1), faces U R F D L B.
export function applyMove(state, m) {
  let cur = state.split("");
  const face = Math.floor(m / 3);
  for (let t = 0; t <= m % 3; t++) {
    const next = cur.slice();
    DEST[face].forEach((d, i) => { next[d] = cur[i]; });
    cur = next;
  }
  return cur.join("");
}

export const moveName = (m) => FACES[Math.floor(m / 3)] + ["", "2", "'"][m % 3];
export const invertMove = (m) => Math.floor(m / 3) * 3 + (2 - (m % 3));

export function parseAlg(s) {
  const out = [];
  for (const tok of s.trim().split(/\s+/).filter(Boolean)) {
    const f = FACES.indexOf(tok[0]);
    const suf = tok.slice(1);
    const p = { "": 0, "2": 1, "'": 2, "’": 2, "3": 2 }[suf];
    if (f < 0 || p === undefined || tok.length > 2) throw new Error("bad move: " + tok);
    out.push(f * 3 + p);
  }
  return out;
}

export function randomMoves(n) {
  const out = [];
  while (out.length < n) {
    const m = Math.floor(Math.random() * 18);
    const last = out[out.length - 1];
    if (last !== undefined) {
      const f = Math.floor(m / 3), lf = Math.floor(last / 3);
      if (f === lf || f + 3 === lf) continue;
    }
    out.push(m);
  }
  return out;
}
