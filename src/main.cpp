#include <raylib.h>
#include "texture_manager.h"
#include "renderer.h"
#include "bitboard.h"

#include "macros.h"

#include <iostream>

int main() {
    const int screenWidth = 800;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "Johnner Chess Engine");
    
    SetTargetFPS(60);

    choco::TextureMgr tMgr;
    choco::Board bb = choco::Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    bb.bitboards[SIDE_WHITE][PAWN] |= (1ULL << E4);
    bb.bitboards[SIDE_BLACK][KNIGHT] |= (1ULL << choco::getIndex(4 - 1, 'E' - 'A'));

    choco::Renderer renderer = choco::Renderer(tMgr);
    renderer.init();

    while (!WindowShouldClose()) {
        renderer.render(bb);
    }
    
    return 0;
}

