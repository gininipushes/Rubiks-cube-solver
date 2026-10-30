#include "cubie.hpp"

#include <algorithm>
#include <sstream>

namespace cube {

namespace {

// The six quarter turns, each written as "what a solved cube looks like
// after this move" (replaced-by notation): after U, slot URF holds UBR.
CubieCube makeBasic(std::array<int, 8> cp, std::array<int, 8> co,
                    std::array<int, 12> ep, std::array<int, 12> eo) {
    CubieCube c;
    c.cp = cp;
    c.co = co;
    c.ep = ep;
    c.eo = eo;
    return c;
}

const std::array<CubieCube, 6>& basicMoves() {
    static const std::array<CubieCube, 6> b = {
        // U
        makeBasic({UBR, URF, UFL, ULB, DFR, DLF, DBL, DRB}, {0, 0, 0, 0, 0, 0, 0, 0},
                  {UB, UR, UF, UL, DR, DF, DL, DB, FR, FL, BL, BR},
                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
        // R
        makeBasic({DFR, UFL, ULB, URF, DRB, DLF, DBL, UBR}, {2, 0, 0, 1, 1, 0, 0, 2},
                  {FR, UF, UL, UB, BR, DF, DL, DB, DR, FL, BL, UR},
                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
        // F
        makeBasic({UFL, DLF, ULB, UBR, URF, DFR, DBL, DRB}, {1, 2, 0, 0, 2, 1, 0, 0},
                  {UR, FL, UL, UB, DR, FR, DL, DB, UF, DF, BL, BR},
                  {0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0}),
        // D
        makeBasic({URF, UFL, ULB, UBR, DLF, DBL, DRB, DFR}, {0, 0, 0, 0, 0, 0, 0, 0},
                  {UR, UF, UL, UB, DF, DL, DB, DR, FR, FL, BL, BR},
                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
        // L
        makeBasic({URF, ULB, DBL, UBR, DFR, UFL, DLF, DRB}, {0, 1, 2, 0, 0, 2, 1, 0},
                  {UR, UF, BL, UB, DR, DF, FL, DB, FR, UL, DL, BR},
                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
        // B
        makeBasic({URF, UFL, UBR, DRB, DFR, DLF, ULB, DBL}, {0, 0, 1, 2, 0, 0, 2, 1},
                  {UR, UF, UL, BR, DR, DF, DL, BL, FR, FL, UB, DB},
                  {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1}),
    };
    return b;
}

// Facelet indices: U1..U9 = 0..8, R 9..17, F 18..26, D 27..35, L 36..44, B 45..53.
constexpr int U_(int i) { return i - 1; }
constexpr int R_(int i) { return 8 + i; }
constexpr int F_(int i) { return 17 + i; }
constexpr int D_(int i) { return 26 + i; }
constexpr int L_(int i) { return 35 + i; }
constexpr int B_(int i) { return 44 + i; }

enum Color { cU, cR, cF, cD, cL, cB };

const int cornerFacelet[8][3] = {
    {U_(9), R_(1), F_(3)}, {U_(7), F_(1), L_(3)}, {U_(1), L_(1), B_(3)}, {U_(3), B_(1), R_(3)},
    {D_(3), F_(9), R_(7)}, {D_(1), L_(9), F_(7)}, {D_(7), B_(9), L_(7)}, {D_(9), R_(9), B_(7)}};
const int edgeFacelet[12][2] = {
    {U_(6), R_(2)}, {U_(8), F_(2)}, {U_(4), L_(2)}, {U_(2), B_(2)},
    {D_(6), R_(8)}, {D_(2), F_(8)}, {D_(4), L_(8)}, {D_(8), B_(8)},
    {F_(6), R_(4)}, {F_(4), L_(6)}, {B_(6), L_(4)}, {B_(4), R_(6)}};
const int cornerColor[8][3] = {{cU, cR, cF}, {cU, cF, cL}, {cU, cL, cB}, {cU, cB, cR},
                               {cD, cF, cR}, {cD, cL, cF}, {cD, cB, cL}, {cD, cR, cB}};
const int edgeColor[12][2] = {{cU, cR}, {cU, cF}, {cU, cL}, {cU, cB}, {cD, cR}, {cD, cF},
                              {cD, cL}, {cD, cB}, {cF, cR}, {cF, cL}, {cB, cL}, {cB, cR}};
const char faceChar[6] = {'U', 'R', 'F', 'D', 'L', 'B'};

template <size_t N>
int permParity(const std::array<int, N>& p) {
    int s = 0;
    for (size_t i = 0; i < N; ++i)
        for (size_t j = i + 1; j < N; ++j)
            if (p[i] > p[j]) ++s;
    return s & 1;
}

}  // namespace

CubieCube CubieCube::multiply(const CubieCube& m) const {
    CubieCube r;
    for (int i = 0; i < 8; ++i) {
        r.cp[i] = cp[m.cp[i]];
        r.co[i] = (co[m.cp[i]] + m.co[i]) % 3;
    }
    for (int i = 0; i < 12; ++i) {
        r.ep[i] = ep[m.ep[i]];
        r.eo[i] = (eo[m.ep[i]] + m.eo[i]) % 2;
    }
    return r;
}

const CubieCube& moveCube(int m) {
    static const std::array<CubieCube, N_MOVES> all = [] {
        std::array<CubieCube, N_MOVES> a;
        for (int f = 0; f < 6; ++f) {
            CubieCube c;
            for (int p = 0; p < 3; ++p) {
                c = c.multiply(basicMoves()[f]);
                a[f * 3 + p] = c;
            }
        }
        return a;
    }();
    return all[m];
}

void CubieCube::move(int m) { *this = multiply(moveCube(m)); }

void CubieCube::apply(const std::vector<int>& alg) {
    for (int m : alg) move(m);
}

bool CubieCube::isSolved() const { return *this == CubieCube(); }

int CubieCube::verify() const {
    std::array<int, 12> seenE{};
    for (int e : ep) {
        if (e < 0 || e > 11) return -2;
        ++seenE[e];
    }
    for (int s : seenE)
        if (s != 1) return -2;
    int sum = 0;
    for (int o : eo) sum += o;
    if (sum % 2) return -3;
    std::array<int, 8> seenC{};
    for (int c : cp) {
        if (c < 0 || c > 7) return -4;
        ++seenC[c];
    }
    for (int s : seenC)
        if (s != 1) return -4;
    sum = 0;
    for (int o : co) sum += o;
    if (sum % 3) return -5;
    if (permParity(cp) != permParity(ep)) return -6;
    return 0;
}

std::string verifyMessage(int code) {
    switch (code) {
        case 0: return "ok";
        case -1: return "each colour must appear exactly 9 times, with 6 distinct centres";
        case -2: return "not all 12 edges exist exactly once";
        case -3: return "one edge is flipped (edge orientation parity)";
        case -4: return "not all 8 corners exist exactly once";
        case -5: return "one corner is twisted (corner orientation parity)";
        case -6: return "two pieces are swapped (permutation parity)";
        default: return "invalid cube";
    }
}

std::string CubieCube::toFacelets() const {
    std::string f(54, '?');
    for (int i = 0; i < 6; ++i) f[9 * i + 4] = faceChar[i];
    for (int i = 0; i < 8; ++i) {
        int j = cp[i], ori = co[i];
        for (int n = 0; n < 3; ++n) f[cornerFacelet[i][(n + ori) % 3]] = faceChar[cornerColor[j][n]];
    }
    for (int i = 0; i < 12; ++i) {
        int j = ep[i], ori = eo[i];
        for (int n = 0; n < 2; ++n) f[edgeFacelet[i][(n + ori) % 2]] = faceChar[edgeColor[j][n]];
    }
    return f;
}

CubieCube CubieCube::fromFacelets(const std::string& s, std::string* err) {
    auto fail = [&](const std::string& msg) {
        if (err) *err = msg;
        return CubieCube();
    };
    if (s.size() != 54) return fail("facelet string must have 54 characters");
    // Map each centre's character to its face.
    int faceOf[256];
    std::fill(std::begin(faceOf), std::end(faceOf), -1);
    for (int i = 0; i < 6; ++i) {
        unsigned char ch = s[9 * i + 4];
        if (faceOf[ch] != -1) return fail(verifyMessage(-1));
        faceOf[ch] = i;
    }
    int f[54], count[6] = {};
    for (int i = 0; i < 54; ++i) {
        int v = faceOf[(unsigned char)s[i]];
        if (v < 0) return fail(verifyMessage(-1));
        f[i] = v;
        ++count[v];
    }
    for (int c : count)
        if (c != 9) return fail(verifyMessage(-1));

    CubieCube c;
    c.cp.fill(-1);
    c.ep.fill(-1);
    for (int i = 0; i < 8; ++i) {
        int ori = 0;
        for (; ori < 3; ++ori)
            if (f[cornerFacelet[i][ori]] == cU || f[cornerFacelet[i][ori]] == cD) break;
        if (ori == 3) return fail(verifyMessage(-4));
        int col1 = f[cornerFacelet[i][(ori + 1) % 3]];
        int col2 = f[cornerFacelet[i][(ori + 2) % 3]];
        for (int j = 0; j < 8; ++j)
            if (col1 == cornerColor[j][1] && col2 == cornerColor[j][2]) {
                c.cp[i] = j;
                c.co[i] = ori;
                break;
            }
        if (c.cp[i] < 0) return fail(verifyMessage(-4));
    }
    for (int i = 0; i < 12; ++i) {
        for (int j = 0; j < 12; ++j) {
            if (f[edgeFacelet[i][0]] == edgeColor[j][0] && f[edgeFacelet[i][1]] == edgeColor[j][1]) {
                c.ep[i] = j;
                c.eo[i] = 0;
                break;
            }
            if (f[edgeFacelet[i][0]] == edgeColor[j][1] && f[edgeFacelet[i][1]] == edgeColor[j][0]) {
                c.ep[i] = j;
                c.eo[i] = 1;
                break;
            }
        }
        if (c.ep[i] < 0) return fail(verifyMessage(-2));
    }
    int v = c.verify();
    if (v != 0) return fail(verifyMessage(v));
    if (err) err->clear();
    return c;
}

CubieCube CubieCube::random(std::mt19937_64& rng) {
    CubieCube c;
    std::shuffle(c.cp.begin(), c.cp.end(), rng);
    std::shuffle(c.ep.begin(), c.ep.end(), rng);
    if (permParity(c.cp) != permParity(c.ep)) std::swap(c.ep[0], c.ep[1]);
    std::uniform_int_distribution<int> d3(0, 2), d2(0, 1);
    int s = 0;
    for (int i = 0; i < 7; ++i) s += (c.co[i] = d3(rng));
    c.co[7] = (3 - s % 3) % 3;
    s = 0;
    for (int i = 0; i < 11; ++i) s += (c.eo[i] = d2(rng));
    c.eo[11] = s % 2;
    return c;
}

std::string moveName(int m) {
    static const char* suffix[3] = {"", "2", "'"};
    return std::string(1, faceChar[m / 3]) + suffix[m % 3];
}

int parseMove(const std::string& tok) {
    if (tok.empty() || tok.size() > 2) return -1;
    int face = -1;
    for (int i = 0; i < 6; ++i)
        if (tok[0] == faceChar[i]) face = i;
    if (face < 0) return -1;
    int p = 0;
    if (tok.size() == 2) {
        if (tok[1] == '2') p = 1;
        else if (tok[1] == '\'' || tok[1] == '3') p = 2;
        else return -1;
    }
    return face * 3 + p;
}

std::vector<int> parseAlg(const std::string& s, std::string* err) {
    std::vector<int> out;
    std::istringstream in(s);
    std::string tok;
    while (in >> tok) {
        int m = parseMove(tok);
        if (m < 0) {
            if (err) *err = "bad move: " + tok;
            return {};
        }
        out.push_back(m);
    }
    if (err) err->clear();
    return out;
}

std::string algToString(const std::vector<int>& alg) {
    std::string s;
    for (size_t i = 0; i < alg.size(); ++i) {
        if (i) s += ' ';
        s += moveName(alg[i]);
    }
    return s;
}

std::vector<int> invertAlg(const std::vector<int>& alg) {
    std::vector<int> inv;
    for (auto it = alg.rbegin(); it != alg.rend(); ++it) inv.push_back((*it / 3) * 3 + (2 - *it % 3));
    return inv;
}

}  // namespace cube
