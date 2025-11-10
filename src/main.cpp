#include <raylib.h>
#include "texture_manager.h"
#include "renderer.h"
#include "bitboard.h"

#include "macros.h"

#include <iostream>

int main() {
    choco::initBitboards();
    choco::Board bb = choco::Board("8/8/8/4R3/8/8/8/8 w - - 0 1");

    std::vector<choco::Move> rookMoves = bb.generateWhiteRookMoves();
    for (const choco::Move& move : rookMoves) {
        std::cout << " - From " << choco::indexToPrettyString(move.from) << " to " << choco::indexToPrettyString(move.to) << std::endl;
    }

    // choco::Board bb = choco::Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // 
    // choco::TextureMgr tMgr;
    // choco::Renderer renderer = choco::Renderer(tMgr);
    // renderer.init();
    // const int screenWidth = 800;
    // const int screenHeight = 800;
    // InitWindow(screenWidth, screenHeight, "Johnner Chess Engine");
    // SetTargetFPS(60);
    // 
    // while (!WindowShouldClose()) {
    //     renderer.render(bb);
    // }
    
    return 0;
}

