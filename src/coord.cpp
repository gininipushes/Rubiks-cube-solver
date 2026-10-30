#include "coord.hpp"

#include <algorithm>
#include <array>

namespace cube::coord {

const int P2_MOVES[N_P2MOVES] = {0, 1, 2, 9, 10, 11, 4, 13, 7, 16};

bool isPhase2Move(int m) {
    for (int x : P2_MOVES)
        if (x == m) return true;
    return false;
}

namespace {

int factorial(int n) { return n <= 1 ? 1 : n * factorial(n - 1); }

// Lehmer-code rank of a permutation of 0..n-1.
template <typename It>
int rankPerm(It p, int n) {
    int r = 0;
    for (int i = 0; i < n; ++i) {
        int smaller = 0;
        for (int j = i + 1; j < n; ++j)
            if (p[j] < p[i]) ++smaller;
        r += smaller * factorial(n - 1 - i);
    }
    return r;
}

std::vector<int> unrankPerm(int r, int n) {
    std::vector<int> avail(n), out(n);
    for (int i = 0; i < n; ++i) avail[i] = i;
    for (int i = 0; i < n; ++i) {
        int f = factorial(n - 1 - i);
        int d = r / f;
        r %= f;
        out[i] = avail[d];
        avail.erase(avail.begin() + d);
    }
    return out;
}

// 4-of-12 position masks, ranked by enumeration order.
struct SliceIndex {
    std::array<int16_t, 4096> rankOf;
    std::vector<int> maskOf;
    SliceIndex() {
        rankOf.fill(-1);
        for (int m = 0; m < 4096; ++m)
            if (__builtin_popcount(m) == 4) {
                rankOf[m] = (int16_t)maskOf.size();
                maskOf.push_back(m);
            }
    }
};

const SliceIndex& sliceIndex() {
    static const SliceIndex s;
    return s;
}

// A representative cube for each coordinate value.
CubieCube twistRep(int t) {
    CubieCube c;
    int s = 0;
    for (int i = 6; i >= 0; --i) {
        c.co[i] = t % 3;
        s += c.co[i];
        t /= 3;
    }
    c.co[7] = (3 - s % 3) % 3;
    return c;
}

CubieCube flipRep(int f) {
    CubieCube c;
    int s = 0;
    for (int i = 10; i >= 0; --i) {
        c.eo[i] = f & 1;
        s += c.eo[i];
        f >>= 1;
    }
    c.eo[11] = s & 1;
    return c;
}

CubieCube sliceRep(int idx) {
    CubieCube c;
    int mask = sliceIndex().maskOf[idx];
    int nextSlice = 8, nextOther = 0;
    for (int i = 0; i < 12; ++i) c.ep[i] = (mask >> i & 1) ? nextSlice++ : nextOther++;
    return c;
}

CubieCube cpermRep(int idx) {
    CubieCube c;
    auto p = unrankPerm(idx, 8);
    for (int i = 0; i < 8; ++i) c.cp[i] = p[i];
    return c;
}

CubieCube udRep(int idx) {
    CubieCube c;
    auto p = unrankPerm(idx, 8);
    for (int i = 0; i < 8; ++i) c.ep[i] = p[i];
    return c;
}

CubieCube spermRep(int idx) {
    CubieCube c;
    auto p = unrankPerm(idx, 4);
    for (int i = 0; i < 4; ++i) c.ep[8 + i] = 8 + p[i];
    return c;
}

template <typename Rep, typename Get>
std::vector<uint16_t> buildMoveTable(int n, Rep rep, Get get, bool phase2Only) {
    std::vector<uint16_t> t((size_t)n * N_MOVES, 0xFFFF);
    for (int i = 0; i < n; ++i) {
        CubieCube c = rep(i);
        for (int m = 0; m < N_MOVES; ++m) {
            if (phase2Only && !isPhase2Move(m)) continue;
            t[(size_t)i * N_MOVES + m] = (uint16_t)get(c.multiply(moveCube(m)));
        }
    }
    return t;
}

// Breadth-first search over the product of two coordinates.
std::vector<uint8_t> buildPrune(int nA, const std::vector<uint16_t>& moveA, int nB,
                                const std::vector<uint16_t>& moveB, int goalA,
                                const std::vector<int>& moves) {
    const size_t total = (size_t)nA * nB;
    std::vector<uint8_t> p(total, 0xFF);
    p[(size_t)goalA * nB + 0] = 0;
    size_t done = 1;
    for (uint8_t depth = 0; done < total; ++depth) {
        for (size_t idx = 0; idx < total; ++idx) {
            if (p[idx] != depth) continue;
            int a = (int)(idx / nB), b = (int)(idx % nB);
            for (int m : moves) {
                size_t nidx = (size_t)moveA[(size_t)a * N_MOVES + m] * nB + moveB[(size_t)b * N_MOVES + m];
                if (p[nidx] == 0xFF) {
                    p[nidx] = depth + 1;
                    ++done;
                }
            }
        }
    }
    return p;
}

}  // namespace

int twist(const CubieCube& c) {
    int t = 0;
    for (int i = 0; i < 7; ++i) t = 3 * t + c.co[i];
    return t;
}

int flip(const CubieCube& c) {
    int f = 0;
    for (int i = 0; i < 11; ++i) f = 2 * f + c.eo[i];
    return f;
}

int slice(const CubieCube& c) {
    int mask = 0;
    for (int i = 0; i < 12; ++i)
        if (c.ep[i] >= FR) mask |= 1 << i;
    return sliceIndex().rankOf[mask];
}

int cornerPerm(const CubieCube& c) { return rankPerm(c.cp.begin(), 8); }
int udEdgePerm(const CubieCube& c) { return rankPerm(c.ep.begin(), 8); }

int slicePerm(const CubieCube& c) {
    int p[4];
    for (int i = 0; i < 4; ++i) p[i] = c.ep[8 + i] - 8;
    return rankPerm(p, 4);
}

const Tables& tables() {
    static const Tables t = [] {
        Tables t;
        t.sliceSolved = slice(CubieCube());
        t.twistMove = buildMoveTable(N_TWIST, twistRep, twist, false);
        t.flipMove = buildMoveTable(N_FLIP, flipRep, flip, false);
        t.sliceMove = buildMoveTable(N_SLICE, sliceRep, slice, false);
        t.cpermMove = buildMoveTable(N_PERM8, cpermRep, cornerPerm, true);
        t.udMove = buildMoveTable(N_PERM8, udRep, udEdgePerm, true);
        t.spermMove = buildMoveTable(N_SPERM, spermRep, slicePerm, true);

        std::vector<int> all(N_MOVES), p2(P2_MOVES, P2_MOVES + N_P2MOVES);
        for (int m = 0; m < N_MOVES; ++m) all[m] = m;
        t.pSliceTwist = buildPrune(N_SLICE, t.sliceMove, N_TWIST, t.twistMove, t.sliceSolved, all);
        t.pSliceFlip = buildPrune(N_SLICE, t.sliceMove, N_FLIP, t.flipMove, t.sliceSolved, all);
        t.pSpermCperm = buildPrune(N_SPERM, t.spermMove, N_PERM8, t.cpermMove, 0, p2);
        t.pSpermUd = buildPrune(N_SPERM, t.spermMove, N_PERM8, t.udMove, 0, p2);
        return t;
    }();
    return t;
}

}  // namespace cube::coord
