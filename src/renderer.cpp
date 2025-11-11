#include "renderer.h"

#include <iostream>
#include "macros.h"

#define LIGHT_SQUARE_COLOR CLITERAL(Color){ 255, 253, 208, 255 }
#define DARK_SQUARE_COLOR  CLITERAL(Color){ 241, 195, 142, 255 }

namespace choco {
    void drawBitboard(uint64_t bitboard, const Texture2D& texture);

    Renderer::Renderer(TextureMgr& textureMgr) : textureMgr(textureMgr) {
    }

    void Renderer::init() const {
        // load textures
        textureMgr.loadAsset("king_white", "assets/king_white.png");
        textureMgr.loadAsset("king_black", "assets/king_black.png");
        textureMgr.loadAsset("queen_white", "assets/queen_white.png");
        textureMgr.loadAsset("queen_black", "assets/queen_black.png");
        textureMgr.loadAsset("bishop_white", "assets/bishop_white.png");
        textureMgr.loadAsset("bishop_black", "assets/bishop_black.png");
        textureMgr.loadAsset("knight_white", "assets/knight_white.png");
        textureMgr.loadAsset("knight_black", "assets/knight_black.png");
        textureMgr.loadAsset("rook_white", "assets/rook_white.png");
        textureMgr.loadAsset("rook_black", "assets/rook_black.png");
        textureMgr.loadAsset("pawn_white", "assets/pawn_white.png");
        textureMgr.loadAsset("pawn_black", "assets/pawn_black.png");
    }

    void Renderer::render(const Board& board) const {
        BeginDrawing();

        ClearBackground(WHITE);

        // checkboard pattern
        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                DrawRectangle((7 - file) * 100, rank * 100, 100, 100, (rank + file) % 2 == 0 ? LIGHT_SQUARE_COLOR : DARK_SQUARE_COLOR);
            }
        }

        // draw all da pieces
        drawBitboard(board.bitboards[SIDE_WHITE][KING],   textureMgr.getAsset("king_white"));
        drawBitboard(board.bitboards[SIDE_WHITE][QUEEN],  textureMgr.getAsset("queen_white"));
        drawBitboard(board.bitboards[SIDE_WHITE][BISHOP], textureMgr.getAsset("bishop_white"));
        drawBitboard(board.bitboards[SIDE_WHITE][KNIGHT], textureMgr.getAsset("knight_white"));
        drawBitboard(board.bitboards[SIDE_WHITE][ROOK],   textureMgr.getAsset("rook_white"));
        drawBitboard(board.bitboards[SIDE_WHITE][PAWN],   textureMgr.getAsset("pawn_white"));

        drawBitboard(board.bitboards[SIDE_BLACK][KING],   textureMgr.getAsset("king_black"));
        drawBitboard(board.bitboards[SIDE_BLACK][QUEEN],  textureMgr.getAsset("queen_black"));
        drawBitboard(board.bitboards[SIDE_BLACK][BISHOP], textureMgr.getAsset("bishop_black"));
        drawBitboard(board.bitboards[SIDE_BLACK][KNIGHT], textureMgr.getAsset("knight_black"));
        drawBitboard(board.bitboards[SIDE_BLACK][ROOK],   textureMgr.getAsset("rook_black"));
        drawBitboard(board.bitboards[SIDE_BLACK][PAWN],   textureMgr.getAsset("pawn_black"));
        

        EndDrawing();
    }

    void drawBitboard(uint64_t bitboard, const Texture2D& texture) {
        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                uint64_t mask = choco::getMask(rank, file);
                if (bitboard & mask) {
                    Rectangle sourceRec = { 0.0f, 0.0f, (float) texture.width, (float) texture.height };
                    Rectangle destRec = { (7 - file) * 100.0f, rank * 100.0f, 100.0f, 100.0f };
                    DrawTexturePro(texture, sourceRec, destRec, { 0.0f, 0.0f }, 0, WHITE);
                }
            }
        }
    }
}
