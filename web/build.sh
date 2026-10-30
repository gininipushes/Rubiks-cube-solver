#!/bin/sh
# Builds web/solver.wasm from the C++ sources.
# Needs clang, lld and a WASI sysroot with libc++
# (Debian/Ubuntu: apt install clang lld wasi-libc libc++-dev-wasm32 libclang-rt-dev-wasm32).
set -e
cd "$(dirname "$0")/.."
clang++ --target=wasm32-wasi --sysroot=/usr \
  -O3 -std=c++17 -fno-exceptions -mexec-model=reactor \
  -Iinclude src/cubie.cpp src/coord.cpp src/solver.cpp web/wasm_api.cpp \
  -Wl,--strip-all -o web/solver.wasm
ls -l web/solver.wasm
