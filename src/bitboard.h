#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace choco {
    class Move {
    public:
        uint8_t from;
        uint8_t to;
    };

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

        void putPiece(uint8_t side, uint8_t piece, uint8_t rank, uint8_t file);
        void removePiece(uint8_t side, uint8_t piece, uint8_t rank, uint8_t file);
        void movePiece(uint8_t side, uint8_t piece, uint8_t rankFrom, uint8_t fileFrom, uint8_t rankTo, uint8_t fileTo);

        std::vector<Move> generateWhitePawnMoves() const;
    };

    uint64_t getMask(int rank, int file);
    uint8_t getIndex(int rank, int file);
} // namespace choco
