// coord.hpp - coordinates, move tables and pruning tables.
//
// A coordinate squeezes one aspect of the cube into a single integer so a
// move becomes an array lookup instead of a cube multiplication.
//
// Phase 1 (reach the subgroup G1 = <U, D, R2, L2, F2, B2>):
//   twist  : corner orientations          0..2186  (3^7)
//   flip   : edge orientations            0..2047  (2^11)
//   slice  : which slots hold FR,FL,BL,BR 0..494   (12 choose 4)
// Phase 2 (solve inside G1, only U, D, R2, L2, F2, B2 allowed):
//   cperm  : corner permutation           0..40319 (8!)
//   udperm : permutation of the 8 U/D edges 0..40319 (8!)
//   sperm  : permutation of the 4 slice edges 0..23 (4!)
#pragma once

#include <cstdint>
#include <vector>

#include "cubie.hpp"

namespace cube::coord {

constexpr int N_TWIST = 2187;
constexpr int N_FLIP = 2048;
constexpr int N_SLICE = 495;
constexpr int N_PERM8 = 40320;
constexpr int N_SPERM = 24;

// Moves allowed in phase 2: U U2 U' D D2 D' R2 L2 F2 B2.
constexpr int N_P2MOVES = 10;
extern const int P2_MOVES[N_P2MOVES];
bool isPhase2Move(int m);

int twist(const CubieCube& c);
int flip(const CubieCube& c);
int slice(const CubieCube& c);
int cornerPerm(const CubieCube& c);
int udEdgePerm(const CubieCube& c);  // only meaningful inside G1
int slicePerm(const CubieCube& c);   // only meaningful inside G1

struct Tables {
    int sliceSolved = 0;  // slice coordinate of the solved cube

    // Move tables: table[coord * 18 + move] -> new coord.
    // Phase-2 tables only fill the columns of phase-2 moves.
    std::vector<uint16_t> twistMove, flipMove, sliceMove;
    std::vector<uint16_t> cpermMove, udMove, spermMove;

    // Pruning tables: exact number of moves needed to solve the pair of
    // coordinates, ignoring everything else. Used as IDA* heuristics.
    std::vector<uint8_t> pSliceTwist;  // slice * N_TWIST + twist
    std::vector<uint8_t> pSliceFlip;   // slice * N_FLIP + flip
    std::vector<uint8_t> pSpermCperm;  // sperm * N_PERM8 + cperm
    std::vector<uint8_t> pSpermUd;     // sperm * N_PERM8 + udperm
};

// Builds the tables on first use (well under a second with -O2).
const Tables& tables();

}  // namespace cube::coord
