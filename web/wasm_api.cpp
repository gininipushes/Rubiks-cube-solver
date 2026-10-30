// wasm_api.cpp - a tiny C interface so the browser can call the solver.
// Built with clang --target=wasm32-wasi (see web/build.sh).
#include <cstring>
#include <random>
#include <string>

#include "coord.hpp"
#include "solver.hpp"

using namespace cube;

static char g_buf[4096];
static std::mt19937_64 g_rng(12345);

extern "C" {

__attribute__((export_name("buffer"))) char* buffer() { return g_buf; }

__attribute__((export_name("init"))) void init(unsigned seed) {
    g_rng.seed(seed);
    coord::tables();
}

static int writeOut(const std::string& s) {
    std::size_t n = std::min(s.size(), sizeof(g_buf) - 1);
    std::memcpy(g_buf, s.data(), n);
    g_buf[n] = 0;
    return (int)n;
}

// Input: facelet string in buffer. Output: solution (or "Error: ...") in buffer.
// Returns the solution length, or -1 on error.
__attribute__((export_name("solve"))) int solve_facelets(int targetLength, int timeMs) {
    std::string facelets(g_buf);
    SolveOptions opt;
    opt.targetLength = targetLength;
    opt.timeLimitSec = timeMs / 1000.0;
    SolveResult r = solveFacelets(facelets, opt);
    if (!r.ok) {
        writeOut("Error: " + r.error);
        return -1;
    }
    writeOut(algToString(r.moves));
    return (int)r.moves.size();
}

// Writes a random-state cube's facelet string to the buffer.
__attribute__((export_name("randomCube"))) int random_cube() {
    return writeOut(CubieCube::random(g_rng).toFacelets());
}

// Input: algorithm in buffer. Output: resulting facelet string, or "Error: ...".
__attribute__((export_name("applyAlg"))) int apply_alg() {
    std::string err;
    auto alg = parseAlg(g_buf, &err);
    if (!err.empty()) return writeOut("Error: " + err), -1;
    CubieCube c;
    c.apply(alg);
    return writeOut(c.toFacelets());
}
}
