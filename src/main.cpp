#include <raylib.h>

#include "texture_manager.h"
#include "renderer.h"
#include "bitboard.h"

#include "macros.h"

#include <iostream>
#include <random>
#include <limits>

void runRookSeedGen() {
    std::random_device rd;
    uint64_t bestIterations = 5911989;
    uint64_t bestSeed = 0;
    uint64_t bestI = 0;

    for (uint64_t i = 0; i < std::numeric_limits<uint64_t>::max(); i++) {
        uint64_t seed = rd();
        uint64_t iterations = choco::initBitboards(seed, bestIterations);
        if (iterations < bestIterations) {
            bestIterations = iterations;
            bestSeed = seed;
            bestI = i;
            std::cout << "\n\n(" << i << ") New best! Seed: " << bestSeed << "; " << iterations << " iterations\n" << std::endl;
        } else {
            std::cout << i << "..." << std::flush;
        }

        if (i - bestI > 10000) {
            break;
        }
    }

    std::cout << "\n\nFinished at attempt " << bestI << " with seed " << bestSeed << " and " << bestIterations << " iterations" << std::endl;
}

void runMoveGenTest() {
    uint64_t iterations = choco::initBitboards(459371994, std::numeric_limits<uint64_t>::max());

    std::cout << "Iterations: " << iterations << std::endl;

    choco::Board bb = choco::Board("8/8/8/4R2P/8/8/8/8 w - - 0 1");

    std::vector<choco::Move> rookMoves = bb.generateWhiteRookMoves();
    for (const choco::Move& move : rookMoves) {
        std::cout << " - " << choco::indexToPrettyString(move.from) << " to " << choco::indexToPrettyString(move.to) << std::endl;
    }
}

void runRenderer() {
    const int screenWidth = 800;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Johnner Chess Engine");
    SetTargetFPS(60);

    choco::Board bb = choco::Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    choco::TextureMgr tMgr;
    choco::Renderer renderer = choco::Renderer(tMgr);
    renderer.init();
    
    while (!WindowShouldClose()) {
        renderer.render(bb);
    }
}

int main() {
    runMoveGenTest();

    return 0;
}

