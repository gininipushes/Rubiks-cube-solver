# Rubiks-cube-solver

A fast Rubik's cube solver in modern C++, built from scratch on **Kociemba's two-phase algorithm**. It turns any scrambled 3x3x3 into a solution of 20 moves or fewer in tens of milliseconds, with no external dependencies.

```
$ ./cubesolver -s "R U R' U' F2 D L2 B U' R2" --length 10
facelets: FFFRUUUDURFDDRLFFRLBFBFRRLLUDDUDDBBDLBBULLDLBRRUFBRBUL
R2 U B' L2 D' F2 U R U' R'
10 moves, 8.418 ms
```

By default it stops at the first solution of 20 moves or fewer. Ask for a shorter target with `--length` and it keeps searching until it finds one or hits the time limit.

## Results

Random-state cubes (uniformly random positions, like WCA scrambles), 1 s limit, one core:

| Target length | Average length | Median time | Average time |
|---|---|---|---|
| 22 moves | 21.6 | 2.8 ms | 4.0 ms |
| 21 moves | 20.8 | 5.0 ms | 11 ms |
| 20 moves | 20.0 | 54 ms | 245 ms |

Lookup tables build in about 0.16 s at startup. The superflip, one of the hardest positions (exactly 20 moves needed), solves optimally in about 3 s.

*Measured with `cubesolver -b 50 --seed 7`; your numbers will vary with hardware.*

## Build

Needs a C++17 compiler and CMake 3.14+.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/tests          # runs ~23,000 checks
```

## Usage

```bash
# Solve from a scramble
./build/cubesolver -s "F R U' R' U' R U R' F' R U R' U' R' F R F'"

# Solve a facelet string (54 chars, faces in U R F D L B order)
./build/cubesolver DUUBULDBFRBFRRULLLBRDFFFBLURDBFDFDRFRULBLUFDURRBLBDUDL

# Random-state cube, or a benchmark
./build/cubesolver --random
./build/cubesolver --bench 100 --length 21 --time 0.5
```

| Option | Meaning |
|---|---|
| `-s, --scramble "ALG"` | solve the state reached by ALG |
| `-r, --random` | solve one random-state cube |
| `-b, --bench N` | solve N random cubes and print stats |
| `-l, --length N` | stop once a solution this short is found (default 20) |
| `-t, --time SEC` | per-solve time limit, best-so-far returned (default 1.0) |
| `--seed N` | RNG seed for reproducible runs |

### Facelet format

Kociemba's standard layout. Each face is read left-to-right, top-to-bottom as it appears in this net. Any six distinct characters work (for example `WRGYOB`); each centre's character defines its face.

```
             |U1 U2 U3|
             |U4 U5 U6|
             |U7 U8 U9|
    |L1 L2 L3|F1 F2 F3|R1 R2 R3|B1 B2 B3|
    |L4 L5 L6|F4 F5 F6|R4 R5 R6|B4 B5 B6|
    |L7 L8 L9|F7 F8 F9|R7 R8 R9|B7 B8 B9|
             |D1 D2 D3|
             |D4 D5 D6|
             |D7 D8 D9|
```

String order: `U1..U9 R1..R9 F1..F9 D1..D9 L1..L9 B1..B9`.

## How it works

The full walkthrough is in [docs/HOW_IT_WORKS.md](docs/HOW_IT_WORKS.md). In short:

1. **Cubie model** (`cubie.cpp`): the cube is 8 corners and 12 edges, each with a position and an orientation. A move is a permutation plus orientation changes, so applying one is a few array lookups.
2. **Coordinates** (`coord.cpp`): parts of the state are compressed into integers (corner twist 0..2186, edge flip 0..2047, middle-slice edge positions 0..494, and three permutations for phase 2). Move tables turn every move into a single table lookup.
3. **Pruning tables**: breadth-first search over pairs of coordinates records the exact distance to solved for each pair. That distance is a lower bound on the real distance, which makes it a perfect IDA\* heuristic.
4. **Two-phase search** (`solver.cpp`):
   - *Phase 1* uses IDA\* to reach the subgroup G1 = ⟨U, D, R2, L2, F2, B2⟩, where every piece is oriented and the middle-layer edges are in the middle layer.
   - *Phase 2* solves the rest using only G1 moves.
   - The search keeps trying longer phase-1 sequences, since a slightly longer phase 1 often unlocks a much shorter phase 2, until the total reaches the target length or time runs out.

## Project layout

```
include/   cubie.hpp  coord.hpp  solver.hpp
src/       cubie.cpp  coord.cpp  solver.cpp  main.cpp
tests/     tests.cpp
docs/      HOW_IT_WORKS.md
```

## Roadmap

- [ ] Symmetry-reduced phase-1 coordinates (16-fold) for stronger pruning
- [ ] Search from all three axes and the inverse cube in parallel
- [ ] Optimal solver mode (Korf-style IDA\* with larger tables)
- [ ] Python bindings and a small web visualiser

## References

- Herbert Kociemba, [Two-Phase Algorithm Details](https://kociemba.org/math/imptwophase.htm)
- Tomas Rokicki et al., [God's Number is 20](https://www.cube20.org/) (2010)
- Richard Korf, *Finding Optimal Solutions to Rubik's Cube Using Pattern Databases* (1997)

## License

MIT, see [LICENSE](LICENSE).
