// solver.hpp - Kociemba's two-phase algorithm.
//
// Phase 1 searches (IDA*) for move sequences that bring the cube into the
// subgroup G1 = <U, D, R2, L2, F2, B2>, where every corner and edge is
// correctly oriented and the four middle-layer edges are in the middle
// layer. Phase 2 then solves the cube using only G1 moves. The search
// keeps trying longer phase-1 sequences, which often allow much shorter
// phase-2 sequences, until the total length hits the target or the time
// limit runs out.
#pragma once

#include <string>
#include <vector>

#include "cubie.hpp"

namespace cube {

struct SolveOptions {
    int targetLength = 20;     // stop as soon as a solution this short is found
    double timeLimitSec = 1.0; // after this, return the best solution so far
};

struct SolveResult {
    bool ok = false;
    std::string error;
    std::vector<int> moves;
    double seconds = 0;
    long long nodes = 0;
};

SolveResult solve(const CubieCube& c, const SolveOptions& opt = {});
SolveResult solveFacelets(const std::string& facelets, const SolveOptions& opt = {});

}  // namespace cube
