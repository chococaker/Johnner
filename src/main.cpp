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
    choco::Board bb = choco::Board("8/6k1/R7/1R6/8/8/8/4K3 w - - 0 1");

    while (true) {
        choco::Engine engine(bb);
        choco::Move move = engine.getBestMove(5);
        std::cout << (bb.state.activeColor == SIDE_WHITE ? "White" : "Black") << " to move: ";
        bb.makeMove(move);
        std::cout << choco::indexToPrettyString(move.from) << " to " << choco::indexToPrettyString(move.to) << std::endl;
        std::cout << choco::boardToPrettyString(bb) << "\n" << std::endl;
    }
}

void runMoveGenTest() {
    choco::initBitboards();

    choco::Board bb = choco::Board("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");

    std::vector<choco::Move> moves;
    bb.addKingMoves(moves);
    std::cout << std::endl;
    uint64_t bitboard = 0;
    for (const choco::Move& move : moves) {
        std::cout << " - " << choco::indexToPrettyString(move.from) << " to " << choco::indexToPrettyString(move.to) << std::endl;
        bitboard |= choco::getMask(move.to);
    }

    bb.makeMove({KING, E1, H1});

    std::cout << choco::bitboardToPrettyString(bitboard) << std::endl;
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
    runEvaluator();
    return 0;
}
