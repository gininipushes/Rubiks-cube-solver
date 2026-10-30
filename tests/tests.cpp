// Minimal self-contained test runner (no external framework needed).
#include <cstdio>
#include <random>
#include <string>

#include "coord.hpp"
#include "solver.hpp"

using namespace cube;

static int failures = 0, checks = 0;
#define CHECK(cond)                                                       \
    do {                                                                  \
        ++checks;                                                         \
        if (!(cond)) {                                                    \
            ++failures;                                                   \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                 \
    } while (0)

static CubieCube after(const std::string& alg) {
    CubieCube c;
    c.apply(parseAlg(alg));
    return c;
}

static void testMoves() {
    std::puts("moves");
    for (int f = 0; f < 6; ++f) {
        CubieCube c;
        for (int i = 0; i < 4; ++i) c.move(f * 3);
        CHECK(c.isSolved());
        CubieCube d;
        d.move(f * 3 + 1);
        d.move(f * 3 + 1);
        CHECK(d.isSolved());
        CubieCube e;
        e.move(f * 3);
        e.move(f * 3 + 2);
        CHECK(e.isSolved());
    }
    // The "sexy move" has order 6.
    CubieCube c;
    for (int i = 1; i <= 6; ++i) {
        c.apply(parseAlg("R U R' U'"));
        CHECK(c.isSolved() == (i == 6));
    }
    // R U has order exactly 105.
    CubieCube d;
    int order = 0;
    do {
        d.apply(parseAlg("R U"));
        ++order;
    } while (!d.isSolved() && order < 1000);
    CHECK(order == 105);
}

static void testFacelets() {
    std::puts("facelets");
    CHECK(CubieCube().toFacelets() == "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB");
    CHECK(after("R").toFacelets() == "UUFUUFUUFRRRRRRRRRFFDFFDFFDDDBDDBDDBLLLLLLLLLUBBUBBUBB");
    CHECK(after("U").toFacelets() == "UUUUUUUUUBBBRRRRRRRRRFFFFFFDDDDDDDDDFFFLLLLLLLLLBBBBBB");
    std::mt19937_64 rng(1);
    for (int i = 0; i < 200; ++i) {
        CubieCube c = CubieCube::random(rng);
        std::string err;
        CHECK(CubieCube::fromFacelets(c.toFacelets(), &err) == c);
        CHECK(err.empty());
    }
    // Colour letters other than URFDLB work as long as centres are distinct.
    std::string s = after("F R").toFacelets();
    const std::string from = "URFDLB", to = "WRGYOB";
    for (char& ch : s) ch = to[from.find(ch)];
    std::string err;
    CHECK(CubieCube::fromFacelets(s, &err) == after("F R"));
    // Invalid cubes are rejected.
    std::string twisted = CubieCube().toFacelets();
    std::swap(twisted[8], twisted[9]);  // twist URF in place (U9 <-> R1 <-> F3)
    std::swap(twisted[9], twisted[20]);
    CubieCube::fromFacelets(twisted, &err);
    CHECK(!err.empty());
}

static void testInvariants() {
    std::puts("invariants");
    std::mt19937_64 rng(2);
    std::uniform_int_distribution<int> mv(0, N_MOVES - 1);
    for (int i = 0; i < 500; ++i) {
        std::vector<int> alg(25);
        for (int& m : alg) m = mv(rng);
        CubieCube c;
        c.apply(alg);
        CHECK(c.verify() == 0);
        c.apply(invertAlg(alg));
        CHECK(c.isSolved());
    }
}

static void testCoordinates() {
    std::puts("coordinates");
    const auto& t = coord::tables();
    std::mt19937_64 rng(3);
    for (int i = 0; i < 300; ++i) {
        CubieCube c = CubieCube::random(rng);
        for (int m = 0; m < N_MOVES; ++m) {
            CubieCube d = c.multiply(moveCube(m));
            CHECK(t.twistMove[coord::twist(c) * N_MOVES + m] == coord::twist(d));
            CHECK(t.flipMove[coord::flip(c) * N_MOVES + m] == coord::flip(d));
            CHECK(t.sliceMove[coord::slice(c) * N_MOVES + m] == coord::slice(d));
            CHECK(t.cpermMove[coord::cornerPerm(c) * N_MOVES + m] == coord::cornerPerm(d) ||
                  !coord::isPhase2Move(m));
        }
    }
}

static void testSolver() {
    std::puts("solver");
    std::mt19937_64 rng(4);
    SolveOptions opt;
    for (int i = 0; i < 100; ++i) {
        CubieCube c = CubieCube::random(rng);
        SolveResult r = solve(c, opt);
        CHECK(r.ok);
        c.apply(r.moves);
        CHECK(c.isSolved());
        CHECK(r.moves.size() <= 22);
    }
    // Superflip: every edge flipped, needs exactly 20 moves.
    CubieCube sf;
    sf.eo.fill(1);
    opt.timeLimitSec = 10;
    SolveResult r = solve(sf, opt);
    CHECK(r.ok);
    sf.apply(r.moves);
    CHECK(sf.isSolved());
    CHECK(r.moves.size() == 20);
    std::printf("  superflip: %s (%.1f ms)\n", algToString(r.moves).c_str(), r.seconds * 1000);

    CHECK(solve(CubieCube()).moves.empty());
    CHECK(!solveFacelets("not a cube").ok);
}

int main() {
    testMoves();
    testFacelets();
    testInvariants();
    testCoordinates();
    testSolver();
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
