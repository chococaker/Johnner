#include <raylib.h>

#include "texture_manager.h"
#include "renderer.h"
#include "board.h"
#include "engine.h"

#include "macros.h"

#include <iostream>
#include <random>
#include <limits>

void runEvaluator() {
    choco::initBitboards();
    choco::initTT();

    choco::Board bb = choco::Board("qqqkqqqq/8/8/8/8/8/8/QQQKQQQQ w - - 0 1");

    std::cout << (bb.state.activeColor == SIDE_WHITE ? "White" : "Black") << " to move: " << std::endl;
    choco::Engine engine(bb);
    choco::Move move = engine.getBestMove(6);
    bb.makeMove(move);

    std::cout << choco::pieceToPrettyString(move.pieceType)
              << " "
              << choco::indexToPrettyString(move.from)
              << " to " << choco::indexToPrettyString(move.to);
    
    if (IS_VALID_PIECE(move.promotionType)) {
        std::cout << " (" << choco::pieceToPrettyString(move.promotionType) << ")";
    }
    std::cout << std::endl;
    
    std::cout << choco::boardToPrettyString(bb) << "\n" << std::endl;

    std::cout << "Done" << std::endl;
}

void runMoveGenTest() {
    choco::initBitboards();

    choco::Board bb = choco::Board("rnb1k1nr/p1pp1ppp/1p6/2P5/2P1p3/5N2/PB1PPPPP/1N2KB1R w KAkq - 0 1");

    std::cout << choco::boardToPrettyString(bb) << "\n" << std::endl;
}

void runRenderer() {
    const int screenWidth = 800;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Johnner Chess Engine");
    SetTargetFPS(60);

    choco::Board bb = choco::Board("r1bqkb1r/ppp2ppp/2np1n2/1B2pQ2/4P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1");
    
    choco::TextureMgr tMgr;
    choco::Renderer renderer = choco::Renderer(tMgr);
    renderer.init();
    
    while (!WindowShouldClose()) {
        renderer.render(bb);
    }
}

int main() {
    runEvaluator();
    return 0;
}
