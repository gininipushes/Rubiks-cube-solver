// cubesolver - command-line front end for the two-phase solver.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "coord.hpp"
#include "solver.hpp"

using namespace cube;

static void usage() {
    std::cout <<
        "Usage: cubesolver [options] [FACELETS]\n"
        "\n"
        "Solves a 3x3x3 Rubik's cube with Kociemba's two-phase algorithm.\n"
        "\n"
        "Input (pick one):\n"
        "  FACELETS                54-char facelet string in URFDLB order\n"
        "  -s, --scramble \"ALG\"    solve the state reached by applying ALG\n"
        "  -r, --random            solve one random-state cube\n"
        "  -b, --bench N           solve N random-state cubes and print stats\n"
        "\n"
        "Options:\n"
        "  -l, --length N          target solution length (default 20)\n"
        "  -t, --time SEC          time limit per solve in seconds (default 1.0)\n"
        "      --seed N            RNG seed for --random / --bench\n"
        "  -h, --help              show this help\n"
        "\n"
        "Example:\n"
        "  cubesolver -s \"R U R' U' F2 D L2 B\"\n"
        "  cubesolver DUUBULDBFRBFRRULLLBRDFFFBLURDBFDFDRFRULBLUFDURRBLBDUDL\n";
}

static void printResult(const SolveResult& r) {
    if (!r.ok) {
        std::cerr << "error: " << r.error << "\n";
        return;
    }
    std::cout << (r.moves.empty() ? "(already solved)" : algToString(r.moves)) << "\n";
    std::printf("%zu moves, %.3f ms\n", r.moves.size(), r.seconds * 1000);
}

int main(int argc, char** argv) {
    SolveOptions opt;
    std::string facelets, scramble;
    bool random = false;
    int bench = 0;
    unsigned long long seed = std::random_device{}();

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "missing value for " << a << "\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "-h" || a == "--help") { usage(); return 0; }
        else if (a == "-s" || a == "--scramble") scramble = next();
        else if (a == "-r" || a == "--random") random = true;
        else if (a == "-b" || a == "--bench") bench = std::atoi(next().c_str());
        else if (a == "-l" || a == "--length") opt.targetLength = std::atoi(next().c_str());
        else if (a == "-t" || a == "--time") opt.timeLimitSec = std::atof(next().c_str());
        else if (a == "--seed") seed = std::strtoull(next().c_str(), nullptr, 10);
        else if (!a.empty() && a[0] == '-') { std::cerr << "unknown option " << a << "\n"; usage(); return 2; }
        else facelets = a;
    }

    auto t0 = std::chrono::steady_clock::now();
    coord::tables();
    double initMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();

    std::mt19937_64 rng(seed);

    if (bench > 0) {
        std::printf("tables built in %.0f ms\n", initMs);
        std::vector<double> times;
        std::vector<int> lens;
        for (int i = 0; i < bench; ++i) {
            CubieCube c = CubieCube::random(rng);
            SolveResult r = solve(c, opt);
            CubieCube check = c;
            check.apply(r.moves);
            if (!r.ok || !check.isSolved()) {
                std::cerr << "FAILED on " << c.toFacelets() << "\n";
                return 1;
            }
            times.push_back(r.seconds * 1000);
            lens.push_back((int)r.moves.size());
        }
        std::sort(times.begin(), times.end());
        double sumT = 0, sumL = 0;
        for (double t : times) sumT += t;
        for (int l : lens) sumL += l;
        int maxL = *std::max_element(lens.begin(), lens.end());
        std::printf("%d cubes, target %d moves, limit %.2f s\n", bench, opt.targetLength, opt.timeLimitSec);
        std::printf("avg length %.2f (max %d)\n", sumL / bench, maxL);
        std::printf("time  avg %.2f ms  median %.2f ms  max %.2f ms\n", sumT / bench, times[times.size() / 2],
                    times.back());
        return 0;
    }

    CubieCube c;
    if (!scramble.empty()) {
        std::string err;
        auto alg = parseAlg(scramble, &err);
        if (!err.empty()) { std::cerr << "error: " << err << "\n"; return 2; }
        c.apply(alg);
        std::cout << "facelets: " << c.toFacelets() << "\n";
    } else if (random) {
        c = CubieCube::random(rng);
        std::cout << "facelets: " << c.toFacelets() << "\n";
    } else if (!facelets.empty()) {
        std::string err;
        c = CubieCube::fromFacelets(facelets, &err);
        if (!err.empty()) { std::cerr << "error: " << err << "\n"; return 2; }
    } else {
        usage();
        return 2;
    }
    SolveResult r = solve(c, opt);
    printResult(r);
    return r.ok ? 0 : 1;
}
