# How the solver works

This walks through the solver from the ground up, in the same order as the code.

## 1. Modelling the cube (`cubie.cpp`)

Stickers are a poor representation: 54 values, most combinations impossible. Instead we track **pieces**. Centres never move, so a state is:

| Array | Size | Values | Meaning |
|---|---|---|---|
| `cp` | 8 | 0..7 | which corner sits in slot *i* |
| `co` | 8 | 0..2 | that corner's twist |
| `ep` | 12 | 0..11 | which edge sits in slot *i* |
| `eo` | 12 | 0..1 | that edge's flip |

Slots are named `URF UFL ULB UBR DFR DLF DBL DRB` for corners and `UR UF UL UB DR DF DL DB FR FL BL BR` for edges.

A move is itself a cube state: "what a solved cube looks like after this move". Applying move `m` to state `s`:

```
new.cp[i] = s.cp[m.cp[i]]
new.co[i] = (s.co[m.cp[i]] + m.co[i]) % 3
new.ep[i] = s.ep[m.ep[i]]
new.eo[i] = (s.eo[m.ep[i]] + m.eo[i]) % 2
```

Slot *i* receives whatever was in slot `m.cp[i]`, carrying its old twist plus the twist the move adds.

## 2. Why only some states are reachable

Of the 8!·3^8·12!·2^12 piece arrangements, only 1 in 12 can be reached by turning faces:

- **Corner twist**: every move preserves `sum(co) mod 3`, so the last corner's twist is determined by the other seven (÷3).
- **Edge flip**: every move preserves `sum(eo) mod 2` (÷2).
- **Permutation parity**: a quarter turn is a 4-cycle on corners *and* a 4-cycle on edges, both odd, so the corner and edge permutations always have equal parity (÷2).

That leaves 43,252,003,274,489,856,000 states. `CubieCube::verify()` checks exactly these three conditions, which is how the solver rejects a cube with a twisted corner or a flipped edge.

## 3. Why brute force fails

There are 18 moves (6 faces × 3 turns). Skipping pointless pairs (`U U`, or `D U` when `U D` does the same) leaves about 13.35 choices per move. Twenty moves deep, that's around 10^22 sequences. Breadth-first search runs out of memory by depth 7 or 8.

## 4. IDA*

Iterative-deepening A* does a depth-first search with a cost bound, raising the bound by one each round. At each node it asks a **heuristic** `h(state)`: a guaranteed lower bound on moves remaining. If `moves so far + h > bound`, the branch is cut. Memory use is tiny (just the current path), and with a good heuristic most of the tree is never visited.

The whole game is getting a strong, cheap heuristic.

## 5. Coordinates and move tables (`coord.cpp`)

A **coordinate** compresses one aspect of the cube into an integer:

| Coordinate | Range | What it captures |
|---|---|---|
| twist | 0..2186 | orientation of the first 7 corners (base 3) |
| flip | 0..2047 | orientation of the first 11 edges (base 2) |
| slice | 0..494 | which 4 slots hold the middle-layer edges (12 choose 4) |
| cperm | 0..40319 | corner permutation (Lehmer code) |
| udperm | 0..40319 | permutation of the 8 top/bottom edges |
| sperm | 0..23 | permutation of the 4 middle-layer edges |

For each coordinate value we build one representative cube, apply each of the 18 moves, and record the resulting coordinate. That gives a **move table**: `twistMove[t*18 + m]`. During search, a move is three table lookups instead of a cube multiplication.

## 6. Pruning tables

Take two coordinates, say slice and twist (495 × 2187 ≈ 1.08 million combinations). Run breadth-first search from the solved pair using the move tables and store the depth at which each pair is first reached. That number is the exact distance to solve *those two aspects*, so it never overestimates the distance for the whole cube. It's an admissible heuristic, and a lookup costs one memory read.

We build four tables, two per phase, and take the max of each pair as `h`.

## 7. Kociemba's two-phase algorithm (`solver.cpp`)

Solving the whole cube with IDA* directly needs huge tables. Kociemba split the problem around a subgroup:

**G1 = ⟨U, D, R2, L2, F2, B2⟩**: the states reachable with only those ten moves. In G1, every corner and edge is correctly oriented, and the four middle-layer edges are in the middle layer. Notice in the move tables that U and D never change orientation and R2, L2, F2, B2 undo their own flips and twists. That's why G1 exists.

- **Phase 1**: IDA* over (twist, flip, slice) until all three are solved, so the cube is in G1. At most 12 moves are ever needed.
- **Phase 2**: IDA* over (cperm, udperm, sperm) using only the 10 G1 moves. At most 18 moves.

Two short searches in spaces of about 2·10^9 and 2·10^10 states, instead of one search over 4·10^19.

### Finding short solutions

The first phase-1 + phase-2 solution is usually 22–24 moves. The trick is to keep going: after each phase-1 solution of length *n*, phase 2 is only allowed `best − n − 1` moves. A longer or different phase 1 often leaves the cube much closer to solved for phase 2, so the total keeps dropping. The solver stops once it hits the target length (default 20) or the time limit, and returns the best solution found.

Two small rules avoid wasted work:

- A phase-1 solution must not end in a G1 move; otherwise a shorter phase 1 already reached G1 and was tried.
- Phase 1 never passes through G1 before its last move, for the same reason.

## 8. Testing (`tests/tests.cpp`)

- Each face turned 4 times is solved; `R U R' U'` has order 6; `R U` has order exactly 105.
- Facelet output after `R` and `U` matches the known strings, and facelet ↔ cubie conversion round-trips on random cubes.
- Every move table entry matches the coordinate computed directly from the cube.
- 100 random-state cubes all solve, and every solution is verified by applying it.
- The superflip (all edges flipped) solves in exactly 20 moves.

## Where to go next

- **Symmetry reduction**: the cube has 48 symmetries, 16 of which preserve the U-D axis. Reducing phase-1 coordinates by them shrinks tables 16× and allows far stronger pruning (this is most of what makes min2phase fast).
- **Multi-axis search**: run the search on the cube rotated to each of 3 axes and on its inverse, keeping the best.
- **Optimal solving**: Korf's approach does a single IDA* with much bigger pattern databases and always returns the true shortest solution.
