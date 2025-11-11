#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace choco {
    uint64_t initBitboards(uint64_t seed, uint64_t maxIterations);

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

        void putPiece(uint8_t side, uint8_t piece, uint8_t index);
        void removePiece(uint8_t side, uint8_t piece, uint8_t index);
        void makeMove(const Move& move, uint8_t piece); // also switches turns, sets up game state

        std::vector<Move> generateWhiteRookMoves() const;
        std::vector<Move> generateWhitePawnMoves() const;
    };

    uint64_t getMask(uint8_t index);
    uint64_t getMask(int rank, int file);
    uint64_t getRankMask(int rank);
    uint64_t getFileMask(int file);
    uint8_t getIndex(int rank, int file);
    uint8_t getRank(uint8_t index);
    uint8_t getFile(uint8_t index);

    std::string indexToPrettyString(uint8_t index);
    std::string bitboardToPrettyString(uint64_t bitboard);
} // namespace choco
