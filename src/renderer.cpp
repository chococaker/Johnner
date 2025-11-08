#include "renderer.h"

#include <iostream>

#define LIGHT_SQUARE_COLOR CLITERAL(Color){ 255, 253, 208, 255 }
#define DARK_SQUARE_COLOR  CLITERAL(Color){ 241, 195, 142, 255 }

namespace choco {
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

    void Renderer::render(const Board& bitboard) const {
        BeginDrawing();

        ClearBackground(WHITE);
        for (int col = 0; col < 8; col++) {
            for (int row = 0; row < 8; row++) {
                // square
                Color color = ((col + row) % 2 == 0) ? LIGHT_SQUARE_COLOR : DARK_SQUARE_COLOR;
                DrawRectangle(col * 100, row * 100, 100, 100, color);

                // piece
                PieceType pieceType = bitboard.getPieceType(row, col);
                PieceColor pieceColor = bitboard.getPieceColor(row, col);
                std::string pieceColorString = (pieceColor == PieceColor::SIDE_WHITE) ? "_white" : "_black";

                std::string pieceString = "";

                switch (pieceType) {
                    case PieceType::KING:
                        pieceString = "king";
                    break;
                    case PieceType::QUEEN:
                        pieceString = "queen";
                    break;
                    case PieceType::BISHOP:
                        pieceString = "bishop";
                    break;
                    case PieceType::KNIGHT:
                        pieceString = "knight";
                    break;
                    case PieceType::ROOK:
                        pieceString = "rook";
                    break;
                    case PieceType::PAWN:
                        pieceString = "pawn";
                    break;
                };

                Texture2D texture = textureMgr.getAsset(pieceString + pieceColorString);
                Rectangle sourceRec = { 0.0f, 0.0f, (float) texture.width, (float) texture.height };
                Rectangle destRec = { col * 100.0f, row * 100.0f, 100.0f, 100.0f };
                DrawTexturePro(texture, sourceRec, destRec, { 0.0f, 0.0f }, 0, WHITE);
            }
        }


        EndDrawing();
    }
}
