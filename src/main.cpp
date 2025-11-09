#include <raylib.h>
#include "texture_manager.h"
#include "renderer.h"
#include "bitboard.h"

#include "macros.h"

#include <iostream>

int main() {
    choco::Board bb = choco::Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::cout << "Pawn moves!" << std::endl;
    for (const choco::Move& move : bb.generateWhitePawnMoves()) {
        std::cout << "from " << choco::toRankFilePos(move.from) << " to " << choco::toRankFilePos(move.to) << std::endl;
    }
    std::cout << "End Pawn moves!" << std::endl;

    // choco::TextureMgr tMgr;
    // choco::Renderer renderer = choco::Renderer(tMgr);
    // renderer.init();
    // const int screenWidth = 800;
    // const int screenHeight = 800;
    // InitWindow(screenWidth, screenHeight, "Johnner Chess Engine");
    // SetTargetFPS(60);

    // while (!WindowShouldClose()) {
    //     renderer.render(bb);
    // }
    
    return 0;
}

