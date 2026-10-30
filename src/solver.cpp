#include "solver.hpp"

#include <algorithm>
#include <chrono>

#include "coord.hpp"

namespace cube {

namespace {

using Clock = std::chrono::steady_clock;

// Skip moves that can never be part of a shortest solution:
// the same face twice in a row, or opposite faces in the "wrong" order
// (U D and D U do the same thing, so only allow U then D).
inline bool redundant(int m, int last) {
    if (last < 0) return false;
    int f = m / 3, lf = last / 3;
    return f == lf || f + 3 == lf;
}

class Search {
public:
    Search(const CubieCube& start, const SolveOptions& opt)
        : t_(coord::tables()), start_(start), opt_(opt), begin_(Clock::now()) {}

    SolveResult run() {
        SolveResult r;
        int tw = coord::twist(start_), fl = coord::flip(start_), sl = coord::slice(start_);
        int h = std::max(t_.pSliceTwist[sl * coord::N_TWIST + tw], t_.pSliceFlip[sl * coord::N_FLIP + fl]);
        for (int depth = h; depth <= MAX_PHASE1 && depth < bestLen_ && !stop_; ++depth)
            phase1(tw, fl, sl, 0, depth);
        r.ok = bestLen_ <= MAX_TOTAL;
        r.moves = best_;
        if (!r.ok) r.error = "no solution found";
        r.seconds = std::chrono::duration<double>(Clock::now() - begin_).count();
        r.nodes = nodes_;
        return r;
    }

private:
    static constexpr int MAX_PHASE1 = 20;
    static constexpr int MAX_PHASE2 = 18;
    static constexpr int MAX_TOTAL = 31;

    const coord::Tables& t_;
    CubieCube start_;
    SolveOptions opt_;
    Clock::time_point begin_;

    int sol_[MAX_TOTAL + MAX_PHASE2 + 2] = {};
    std::vector<int> best_;
    int bestLen_ = MAX_TOTAL + 1;
    bool stop_ = false;
    long long nodes_ = 0;

    void checkTime() {
        if (best_.empty() && bestLen_ > MAX_TOTAL) return;  // keep going until we have something
        double el = std::chrono::duration<double>(Clock::now() - begin_).count();
        if (el >= opt_.timeLimitSec) stop_ = true;
    }

    void phase1(int tw, int fl, int sl, int n, int togo) {
        if (togo == 0) {
            if (tw == 0 && fl == 0 && sl == t_.sliceSolved && (n == 0 || !coord::isPhase2Move(sol_[n - 1])))
                phase2(n);
            return;
        }
        if ((++nodes_ & 0xFFF) == 0) checkTime();
        if (stop_) return;
        int last = n ? sol_[n - 1] : -1;
        for (int m = 0; m < N_MOVES; ++m) {
            if (redundant(m, last)) continue;
            int ntw = t_.twistMove[tw * N_MOVES + m];
            int nfl = t_.flipMove[fl * N_MOVES + m];
            int nsl = t_.sliceMove[sl * N_MOVES + m];
            int h = std::max(t_.pSliceTwist[nsl * coord::N_TWIST + ntw], t_.pSliceFlip[nsl * coord::N_FLIP + nfl]);
            // h must fit in the remaining depth; and reaching G1 early would
            // only produce a phase-1 solution we have already tried.
            if (h > togo - 1 || (h == 0 && togo - 1 > 0)) continue;
            sol_[n] = m;
            phase1(ntw, nfl, nsl, n + 1, togo - 1);
            if (stop_) return;
        }
    }

    void phase2(int n1) {
        // Rebuild the cube at the end of phase 1 to get phase-2 coordinates.
        CubieCube c = start_;
        for (int i = 0; i < n1; ++i) c.move(sol_[i]);
        int cp = coord::cornerPerm(c), ud = coord::udEdgePerm(c), sp = coord::slicePerm(c);
        int h = std::max(t_.pSpermCperm[sp * coord::N_PERM8 + cp], t_.pSpermUd[sp * coord::N_PERM8 + ud]);
        int maxD = std::min(bestLen_ - 1 - n1, MAX_PHASE2);
        for (int d = h; d <= maxD; ++d) {
            if (search2(cp, ud, sp, n1, d)) {
                bestLen_ = n1 + d;
                best_.assign(sol_, sol_ + bestLen_);
                if (bestLen_ <= opt_.targetLength) stop_ = true;
                checkTime();
                return;
            }
        }
    }

    bool search2(int cp, int ud, int sp, int n, int togo) {
        if (togo == 0) return cp == 0 && ud == 0 && sp == 0;
        ++nodes_;
        int last = n ? sol_[n - 1] : -1;
        for (int m : coord::P2_MOVES) {
            if (redundant(m, last)) continue;
            int ncp = t_.cpermMove[cp * N_MOVES + m];
            int nud = t_.udMove[ud * N_MOVES + m];
            int nsp = t_.spermMove[sp * N_MOVES + m];
            int h = std::max(t_.pSpermCperm[nsp * coord::N_PERM8 + ncp], t_.pSpermUd[nsp * coord::N_PERM8 + nud]);
            if (h > togo - 1) continue;
            sol_[n] = m;
            if (search2(ncp, nud, nsp, n + 1, togo - 1)) return true;
        }
        return false;
    }
};

}  // namespace

SolveResult solve(const CubieCube& c, const SolveOptions& opt) {
    SolveResult r;
    int v = c.verify();
    if (v != 0) {
        r.error = verifyMessage(v);
        return r;
    }
    if (c.isSolved()) {
        r.ok = true;
        return r;
    }
    return Search(c, opt).run();
}

SolveResult solveFacelets(const std::string& facelets, const SolveOptions& opt) {
    std::string err;
    CubieCube c = CubieCube::fromFacelets(facelets, &err);
    if (!err.empty()) {
        SolveResult r;
        r.error = err;
        return r;
    }
    return solve(c, opt);
}

}  // namespace cube
