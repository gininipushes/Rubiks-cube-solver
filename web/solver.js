// solver.js - loads solver.wasm and wraps its exports.
// Works in browsers and in Node (pass the wasm bytes to loadSolver).
export async function loadSolver(source = "solver.wasm") {
  let memory;
  const view = () => new DataView(memory.buffer);
  const wasi = {
    clock_time_get(_id, _precision, out) {
      const ns = BigInt(Math.round(performance.now() * 1e6));
      view().setBigUint64(out, ns, true);
      return 0;
    },
    environ_sizes_get(countPtr, sizePtr) {
      view().setUint32(countPtr, 0, true);
      view().setUint32(sizePtr, 0, true);
      return 0;
    },
    environ_get() { return 0; },
    fd_write(_fd, _iovs, _n, written) { view().setUint32(written, 0, true); return 0; },
    fd_close() { return 0; },
    fd_seek() { return 0; },
    proc_exit(code) { throw new Error("wasm exited with code " + code); },
  };
  const imports = { wasi_snapshot_preview1: wasi };
  let instance;
  if (typeof source === "string") {
    const res = await fetch(source);
    instance = (await WebAssembly.instantiate(await res.arrayBuffer(), imports)).instance;
  } else {
    instance = (await WebAssembly.instantiate(source, imports)).instance;
  }
  const ex = instance.exports;
  memory = ex.memory;
  if (ex._initialize) ex._initialize();

  const enc = new TextEncoder(), dec = new TextDecoder();
  const write = (s) => {
    const bytes = enc.encode(s + "\0");
    new Uint8Array(memory.buffer, ex.buffer(), bytes.length).set(bytes);
  };
  const read = (n) => dec.decode(new Uint8Array(memory.buffer, ex.buffer(), n));

  const t0 = performance.now();
  ex.init((Math.random() * 2 ** 32) >>> 0);
  const tableMs = performance.now() - t0;

  return {
    tableMs,
    /** Solve a 54-char facelet string. Returns {moves, length, ms} or throws. */
    solve(facelets, targetLength = 20, timeMs = 1000) {
      write(facelets);
      const t = performance.now();
      const len = ex.solve(targetLength, timeMs);
      const ms = performance.now() - t;
      const out = dec.decode(new Uint8Array(memory.buffer, ex.buffer(), 4096)).split("\0")[0];
      if (len < 0) throw new Error(out.replace(/^Error: /, ""));
      return { moves: out, length: len, ms };
    },
    randomCube() { return read(ex.randomCube()); },
    /** Facelet string after applying an algorithm to a solved cube. */
    applyAlg(alg) {
      write(alg);
      const n = ex.applyAlg();
      const out = dec.decode(new Uint8Array(memory.buffer, ex.buffer(), 4096)).split("\0")[0];
      if (n < 0) throw new Error(out.replace(/^Error: /, ""));
      return out;
    },
  };
}
