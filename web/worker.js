// Runs the WebAssembly solver off the main thread so the page never freezes.
import { loadSolver } from "./solver.js";

const ready = loadSolver(new URL("./solver.wasm", import.meta.url).href);

onmessage = async (e) => {
  const { id, type, facelets, target, timeMs } = e.data;
  try {
    const s = await ready;
    if (type === "init") postMessage({ id, ok: true, tableMs: s.tableMs });
    else if (type === "solve") postMessage({ id, ok: true, ...s.solve(facelets, target, timeMs) });
    else if (type === "random") postMessage({ id, ok: true, facelets: s.randomCube() });
  } catch (err) {
    postMessage({ id, ok: false, error: err.message });
  }
};
