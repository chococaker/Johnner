#include <raylib.h>

#include "texture_manager.h"
#include "renderer.h"
#include "bitboard.h"

#include "macros.h"

#include <iostream>
#include <random>
#include <limits>

void runMoveGenTest() {
    choco::initBitboards(459371994, 2595412012);

    choco::Board bb = choco::Board("1PP5/8/2N5/4p3/3P4/8/5P2/8 w - - 0 1");

    std::vector<choco::Move> knightMoves = bb.generateWhiteKnightMoves();
    std::cout << std::endl;
    for (const choco::Move& move : knightMoves) {
        std::cout << " - " << choco::indexToPrettyString(move.from) << " to " << choco::indexToPrettyString(move.to) << std::endl;
    }
}

void runRenderer() {
    const int screenWidth = 800;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Johnner Chess Engine");
    SetTargetFPS(60);

    choco::Board bb = choco::Board("8/8/8/8/8/4B3/8/8 w - - 0 1");
    
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
