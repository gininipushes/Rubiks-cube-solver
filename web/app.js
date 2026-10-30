// app.js - 3D visualiser and UI for the WebAssembly solver.
import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import {
  SOLVED, STICKERS, AXES, FACES, dot, applyMove, moveName, invertMove, parseAlg, randomMoves,
} from "./cubemodel.js";

const COLORS = { U: 0xf5f5f5, R: 0xd62828, F: 0x1fa64a, D: 0xffd60a, L: 0xff7b00, B: 0x1b5fd1, "?": 0x3a3f4b };
const $ = (id) => document.getElementById(id);

// ---------- solver worker ----------
const worker = new Worker(new URL("./worker.js", import.meta.url), { type: "module" });
let nextId = 0;
const pending = new Map();
worker.onmessage = (e) => { pending.get(e.data.id)?.(e.data); pending.delete(e.data.id); };
const call = (msg) => new Promise((res) => { const id = nextId++; pending.set(id, res); worker.postMessage({ id, ...msg }); });

// ---------- state ----------
let state = SOLVED;     // facelet string currently shown
let solution = [];      // moves of the last solve
let solStart = null;    // state the solution starts from
let pos = 0;            // how many solution moves have been applied
let playing = false;
let paintColor = null;
const queue = [];       // moves waiting to animate
let animating = null;

// ---------- three.js scene ----------
const view = $("view");
const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
view.appendChild(renderer.domElement);
const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(35, 1, 0.1, 100);
camera.position.set(5.2, 4.4, 6.6);
const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.enablePan = false;
controls.minDistance = 6;
controls.maxDistance = 16;
scene.add(new THREE.AmbientLight(0xffffff, 1.6));
const sun = new THREE.DirectionalLight(0xffffff, 1.6);
sun.position.set(4, 8, 6);
scene.add(sun);

const root = new THREE.Group();
scene.add(root);
const pivot = new THREE.Group();
root.add(pivot);

function roundedSquare(size, r) {
  const s = size / 2, sh = new THREE.Shape();
  sh.moveTo(-s + r, -s); sh.lineTo(s - r, -s); sh.quadraticCurveTo(s, -s, s, -s + r);
  sh.lineTo(s, s - r); sh.quadraticCurveTo(s, s, s - r, s); sh.lineTo(-s + r, s);
  sh.quadraticCurveTo(-s, s, -s, s - r); sh.lineTo(-s, -s + r); sh.quadraticCurveTo(-s, -s, -s + r, -s);
  return new THREE.ShapeGeometry(sh);
}

const cubies = new Map();  // "x,y,z" -> Group
const bodyGeo = new THREE.BoxGeometry(0.97, 0.97, 0.97);
const bodyMat = new THREE.MeshStandardMaterial({ color: 0x111111, roughness: 0.6 });
for (let x = -1; x <= 1; x++) for (let y = -1; y <= 1; y++) for (let z = -1; z <= 1; z++) {
  if (!x && !y && !z) continue;
  const g = new THREE.Group();
  g.position.set(x, y, z);
  g.userData.home = new THREE.Vector3(x, y, z);
  g.userData.p = [x, y, z];
  g.add(new THREE.Mesh(bodyGeo, bodyMat));
  root.add(g);
  cubies.set([x, y, z].join(), g);
}
const stickerGeo = roundedSquare(0.84, 0.12);
const stickerMeshes = STICKERS.map((s, i) => {
  const m = new THREE.Mesh(stickerGeo, new THREE.MeshStandardMaterial({ roughness: 0.35, side: THREE.DoubleSide }));
  const n = new THREE.Vector3(...s.n);
  m.position.copy(n).multiplyScalar(0.487);
  m.quaternion.setFromUnitVectors(new THREE.Vector3(0, 0, 1), n);
  m.userData.idx = i;
  cubies.get(s.p.join()).add(m);
  return m;
});

function paint() {
  stickerMeshes.forEach((m, i) => m.material.color.setHex(COLORS[state[i]] ?? COLORS["?"]));
  $("faceletOut").textContent = state;
}

function resize() {
  const w = view.clientWidth, h = view.clientHeight;
  renderer.setSize(w, h);
  camera.aspect = w / h;
  camera.updateProjectionMatrix();
}
new ResizeObserver(resize).observe(view);

// ---------- move animation ----------
const ease = (t) => (t < 0.5 ? 2 * t * t : 1 - (-2 * t + 2) ** 2 / 2);

function startNext() {
  if (animating || !queue.length) return;
  const { m, onDone } = queue.shift();
  const axis = AXES[Math.floor(m / 3)];
  const power = (m % 3) + 1;
  const angle = power === 3 ? Math.PI / 2 : -power * Math.PI / 2;
  for (const g of cubies.values()) if (dot(g.userData.p, axis) === 1) pivot.attach(g);
  const dur = +$("speed").value * (power === 2 ? 1.4 : 1);
  animating = { m, axis: new THREE.Vector3(...axis), angle, t0: performance.now(), dur, onDone };
}

function finishAnimation() {
  const { m, onDone } = animating;
  pivot.quaternion.identity();
  pivot.updateMatrixWorld();
  for (const g of [...pivot.children]) {
    root.attach(g);
    g.position.copy(g.userData.home);
    g.quaternion.identity();
  }
  state = applyMove(state, m);
  paint();
  animating = null;
  onDone?.();
  startNext();
}

function enqueue(m, onDone) { queue.push({ m, onDone }); startNext(); }
const busy = () => !!animating || queue.length > 0;

renderer.setAnimationLoop((now) => {
  if (animating) {
    const t = Math.min(1, (now - animating.t0) / animating.dur);
    pivot.quaternion.setFromAxisAngle(animating.axis, animating.angle * ease(t));
    if (t >= 1) finishAnimation();
  }
  controls.update();
  renderer.render(scene, camera);
});

// ---------- UI ----------
const status = (s) => { $("status").textContent = s; };

function clearSolution() {
  solution = []; solStart = null; pos = 0; playing = false;
  $("solution").hidden = true;
}

function setState(s) { queue.length = 0; clearSolution(); state = s; paint(); }

function renderMoves() {
  $("moves").innerHTML = solution.map((m, i) =>
    `<span class="${i < pos ? "done" : i === pos ? "cur" : ""}">${moveName(m)}</span>`).join("");
  $("play").textContent = playing ? "❚❚ Pause" : "▶ Play";
}

function stepForward(cb) {
  if (pos >= solution.length) { playing = false; renderMoves(); return; }
  const m = solution[pos++];
  renderMoves();
  enqueue(m, cb);
}
function stepBack() {
  if (pos <= 0 || busy()) return;
  const m = solution[--pos];
  renderMoves();
  enqueue(invertMove(m));
}
function playLoop() {
  if (!playing) return;
  if (pos >= solution.length) { playing = false; renderMoves(); status("Solved!"); return; }
  stepForward(playLoop);
}

$("randomState").onclick = async () => {
  if (busy()) return;
  const r = await call({ type: "random" });
  setState(r.facelets);
  status("Random-state cube (uniformly random position)");
};
$("scramble").onclick = () => {
  if (busy()) return;
  clearSolution();
  const alg = randomMoves(25);
  const old = $("speed").value;
  $("speed").value = 90;
  alg.forEach((m, i) => enqueue(m, i === alg.length - 1 ? () => { $("speed").value = old; } : null));
  $("algInput").value = alg.map(moveName).join(" ");
  status("Scramble: " + alg.map(moveName).join(" "));
};
$("reset").onclick = () => { setState(SOLVED); status("Solved cube"); };

$("solve").onclick = async () => {
  if (busy()) return;
  if (state.includes("?")) { status("Paint every sticker before solving"); return; }
  if (state === SOLVED) { status("Already solved"); return; }
  if (solStart !== null && pos > 0) { solution = []; }
  $("solve").disabled = true;
  status("Solving…");
  const target = +$("target").value;
  const r = await call({ type: "solve", facelets: state, target, timeMs: 1500 });
  $("solve").disabled = false;
  if (!r.ok) { status("Can't solve: " + r.error); return; }
  solution = parseAlg(r.moves);
  solStart = state;
  pos = 0;
  playing = false;
  $("solution").hidden = false;
  $("meta").textContent = `${r.length} moves · found in ${r.ms.toFixed(1)} ms`;
  renderMoves();
  status(`Solution: ${r.length} moves in ${r.ms.toFixed(1)} ms. Press Play.`);
};

$("play").onclick = () => {
  if (playing) { playing = false; renderMoves(); return; }
  if (pos >= solution.length) {  // replay from the start
    state = solStart; pos = 0; paint();
  }
  playing = true;
  renderMoves();
  if (!busy()) playLoop();
};
$("fwd").onclick = () => { if (!busy()) { playing = false; stepForward(); } };
$("back").onclick = () => { playing = false; stepBack(); };

// Manual turns
FACES.split("").forEach((f, fi) => [0, 2, 1].forEach((p) => {
  const b = document.createElement("button");
  b.textContent = moveName(fi * 3 + p);
  b.onclick = () => { clearSolution(); enqueue(fi * 3 + p); };
  $("moveButtons").appendChild(b);
}));
addEventListener("keydown", (e) => {
  if (e.target.tagName === "INPUT" || e.ctrlKey || e.metaKey || e.altKey) return;
  const f = FACES.indexOf(e.key.toUpperCase());
  if (f < 0) return;
  clearSolution();
  enqueue(f * 3 + (e.shiftKey ? 2 : 0));
});

// Text input
$("applyAlg").onclick = () => {
  try {
    const alg = parseAlg($("algInput").value);
    clearSolution();
    alg.forEach((m) => enqueue(m));
  } catch (err) { status(err.message); }
};
$("loadFacelets").onclick = () => {
  const s = $("faceletInput").value.trim();
  if (s.length !== 54) { status("Facelet string must have 54 characters"); return; }
  // Accept any six letters: map each centre's letter to its face.
  const map = {};
  for (let f = 0; f < 6; f++) map[s[9 * f + 4]] = FACES[f];
  if (Object.keys(map).length !== 6 || [...s].some((c) => !map[c])) { status("Need six distinct centre colours"); return; }
  setState([...s].map((c) => map[c]).join(""));
  status("Loaded facelets");
};

// Paint mode
FACES.split("").forEach((f) => {
  const b = document.createElement("button");
  b.style.background = "#" + COLORS[f].toString(16).padStart(6, "0");
  b.title = f;
  b.onclick = () => {
    paintColor = paintColor === f ? null : f;
    document.querySelectorAll(".palette button").forEach((x) => x.classList.toggle("on", x.title === paintColor));
    status(paintColor ? "Tap stickers to paint them" : "Paint mode off");
  };
  $("palette").appendChild(b);
});
$("clearPaint").onclick = () => {
  setState([...SOLVED].map((c, i) => (i % 9 === 4 ? c : "?")).join(""));
  status("Pick a colour and tap stickers");
};

const ray = new THREE.Raycaster();
let downAt = null;
renderer.domElement.addEventListener("pointerdown", (e) => { downAt = [e.clientX, e.clientY]; });
renderer.domElement.addEventListener("pointerup", (e) => {
  if (!paintColor || !downAt || busy()) return;
  if (Math.hypot(e.clientX - downAt[0], e.clientY - downAt[1]) > 6) return;  // it was a drag
  const r = renderer.domElement.getBoundingClientRect();
  ray.setFromCamera(new THREE.Vector2(((e.clientX - r.left) / r.width) * 2 - 1, -((e.clientY - r.top) / r.height) * 2 + 1), camera);
  const hit = ray.intersectObjects(stickerMeshes)[0];
  if (!hit) return;
  const i = hit.object.userData.idx;
  if (i % 9 === 4) { status("Centres are fixed"); return; }
  clearSolution();
  state = state.slice(0, i) + paintColor + state.slice(i + 1);
  paint();
});

// ---------- go ----------
paint();
resize();
call({ type: "init" }).then((r) => {
  status(r.ok ? `Solver ready · tables built in ${r.tableMs.toFixed(0)} ms` : "Solver failed to load: " + r.error);
});
