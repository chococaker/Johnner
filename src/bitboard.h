#pragma once

#include <cstdint>
#include <iostream>
#include <string>

namespace choco {
    class GameState {
    public:
        uint8_t activeColor;
        uint8_t castling;
        uint8_t halfMoveClock;
        uint8_t enpassantSquare;
        uint16_t moveCount;

        void toggleCastling(int color, int sidePiece);
        bool canCastle(int color, int sidePiece);
    };

    class Board {
    public:
        Board();
        Board(const std::string& fen);

        uint64_t bitboards[2][6];
        GameState state;
    };

    uint64_t getMask(int rank, int file);
    uint8_t getIndex(int rank, int file);
} // namespace choco
