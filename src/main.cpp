#include <raylib.h>

#include "texture_manager.h"
#include "renderer.h"
#include "board.h"

#include "macros.h"

#include <iostream>
#include <random>
#include <limits>

void runMoveGenTest() {
    choco::initBitboards();

    choco::Board bb = choco::Board("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");

    std::vector<choco::Move> moves = bb.generateKingMoves();
    std::cout << std::endl;
    uint64_t bitboard = 0;
    for (const choco::Move& move : moves) {
        std::cout << " - " << choco::indexToPrettyString(move.from) << " to " << choco::indexToPrettyString(move.to) << std::endl;
        bitboard |= choco::getMask(move.to);
    }

    bb.makeMove({E1, H1}, KING);

    std::cout << choco::bitboardToPrettyString(bitboard) << std::endl;
    std::cout << std::to_string(bb.state.castling);
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
    // runRenderer();
    choco::Board bb;

    std::cout << sizeof(bb) << std::endl;

    return 0;
}
