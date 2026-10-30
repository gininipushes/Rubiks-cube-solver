// cubie.hpp - the cubie-level model of a 3x3x3 Rubik's cube.
//
// A cube state is 8 corners + 12 edges, each with a position and an
// orientation. Centres never move, so they are not stored.
#pragma once

#include <array>
#include <random>
#include <string>
#include <vector>

namespace cube {

// Slot / piece names (Kociemba's convention).
enum Corner { URF, UFL, ULB, UBR, DFR, DLF, DBL, DRB };
enum Edge { UR, UF, UL, UB, DR, DF, DL, DB, FR, FL, BL, BR };

// Faces, in move order. Move index = face * 3 + (power - 1),
// so U=0, U2=1, U'=2, R=3, ... B'=17.
enum Face { FU, FR_, FF, FD, FL_, FB };
constexpr int N_MOVES = 18;

struct CubieCube {
    std::array<int, 8> cp{0, 1, 2, 3, 4, 5, 6, 7};  // corner permutation
    std::array<int, 8> co{};                        // corner orientation 0..2
    std::array<int, 12> ep{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};  // edge permutation
    std::array<int, 12> eo{};                       // edge orientation 0..1

    bool operator==(const CubieCube& o) const {
        return cp == o.cp && co == o.co && ep == o.ep && eo == o.eo;
    }
    bool operator!=(const CubieCube& o) const { return !(*this == o); }

    // Returns (*this) followed by m.
    CubieCube multiply(const CubieCube& m) const;
    // Apply move 0..17 in place.
    void move(int m);
    void apply(const std::vector<int>& alg);
    bool isSolved() const;

    // 0 if the state is reachable, otherwise a negative error code
    // (see verifyMessage).
    int verify() const;

    // 54-char facelet string in URFDLB order (Kociemba's format).
    std::string toFacelets() const;
    // Parses a facelet string. Any 6 distinct characters are accepted;
    // each centre's character names that face. On failure returns a
    // solved cube and sets *err.
    static CubieCube fromFacelets(const std::string& s, std::string* err);

    // A uniformly random reachable state ("random-state scramble").
    static CubieCube random(std::mt19937_64& rng);
};

std::string verifyMessage(int code);

const CubieCube& moveCube(int m);  // the cube produced by move m on solved
std::string moveName(int m);
int parseMove(const std::string& tok);  // -1 on failure
std::vector<int> parseAlg(const std::string& s, std::string* err = nullptr);
std::string algToString(const std::vector<int>& alg);
std::vector<int> invertAlg(const std::vector<int>& alg);

}  // namespace cube
