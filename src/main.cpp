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
    choco::Board bb = choco::Board("8/n5p1/5b1p/p6k/N1K2P2/1p3NPP/8/8 w - - 0 2");

    std::cout << (bb.state.activeColor == SIDE_WHITE ? "White" : "Black") << " to move: ";
    choco::Engine engine(bb);
    choco::Move move = engine.getBestMove(8);

    std::cout << choco::indexToPrettyString(move.from) << " to " << choco::indexToPrettyString(move.to) << std::endl;
    std::cout << choco::boardToPrettyString(bb) << "\n" << std::endl;

    std::cout << bb.state.activeColor << std::endl;

    std::cout << "Done" << std::endl;
}

void runMoveGenTest() {
    choco::initBitboards();

    choco::Board bb = choco::Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::vector<choco::Move> moves = bb.generatePLMoves();
    std::cout << choco::boardToPrettyString(bb) << "\n" << std::endl;
    bb.makeMove({PAWN, B2, B3});
    std::cout << choco::boardToPrettyString(bb) << std::endl;
}

void runRenderer() {
    const int screenWidth = 800;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Johnner Chess Engine");
    SetTargetFPS(60);

    choco::Board bb = choco::Board("7K/8/8/7R/6R1/1k6/8/8 w - - 0 1");
    
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
