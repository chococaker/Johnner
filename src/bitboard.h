#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace choco {
    void initBitboards(uint64_t rookSeed, uint64_t bishopSeed);

    class Move {
    public:
        uint8_t from;
        uint8_t to;
        uint8_t promotionType;
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

        void makeMove(const Move& move, uint8_t piece); // also switches turns, sets up game state

        std::vector<Move> generateKingMoves() const;
        std::vector<Move> generateQueenMoves() const;
        std::vector<Move> generateKnightMoves() const;
        std::vector<Move> generateBishopMoves() const;
        std::vector<Move> generateRookMoves() const;
        std::vector<Move> generatePawnMoves() const;

    private:
        void putPiece(uint8_t side, uint8_t piece, uint8_t index);
        void removePiece(uint8_t side, uint8_t piece, uint8_t index);

        uint64_t plKingMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plQueenMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plBishopMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plKnightMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plRookMoveBB(uint8_t square, uint8_t color) const;

        uint64_t attacksToKing() const;
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
