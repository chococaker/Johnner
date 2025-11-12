#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace choco {
    void initBitboards(); // should be called before any move generation

    class Move {
    public:
        Move(uint8_t from, uint8_t to) : from(from), to(to), promotionType(0) { }

        Move(uint8_t from, uint8_t to, uint8_t promotionType) : from(from), to(to), promotionType(promotionType) { }

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

        void enableCastling(uint8_t color, uint8_t sidePiece);
        void disableCastling(uint8_t color, uint8_t sidePiece);
        bool canCastle(uint8_t color, uint8_t sidePiece) const;
    };

    class Board {
    public:
        Board();
        Board(const std::string& fen);

        uint64_t bitboards[2][6];
        GameState state;

        /**
         * @brief Makes a move and sets up game state for the next turn. Move must be pseudo-legal.
         * 
         * @param move the pseudo-legal move
         * @param piece what piece type to move
         * @return whether the move was legal
         */
        bool makeMove(const Move& move, uint8_t piece);

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

        uint64_t getAttacks(uint8_t color) const; // get attacks that a color is doing
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
