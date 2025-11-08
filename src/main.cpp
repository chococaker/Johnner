#include <raylib.h>
#include "texture_manager.h"
#include "renderer.h"
#include "bitboard.h"

#include <iostream>

int main() {
    const int screenWidth = 800;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "Play Galahad");
    
    SetTargetFPS(60);

    // choco::TextureMgr tMgr;
    choco::Board bb = choco::Board();
    bb.w_bitboard[static_cast<int>(choco::PieceType::KING)] |= (1ULL << 28);

    std::cout << bb << std::endl;

    // choco::Renderer renderer = choco::Renderer(tMgr);
    // renderer.init();

    // while (!WindowShouldClose()) {
    //     renderer.render(bb);
    // }
    
    return 0;
}

