#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include "types.h"

namespace choco {
    void initBitboards(); // should be called before any move generation
    
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

    class UnmakeMove {
    public:
        Move move;
        // does not include en passant (it is implied in unmakeMove())
        uint8_t pieceTaken;
        GameState state;

        bool isValid() const;
    };

    extern const UnmakeMove INVALID_MOVE;

    enum class MateStatus {
        ONGOING, STALEMATE, WHITE_WIN, BLACK_WIN
    };

    class Board {
    public:
        Board();
        Board(std::string fen);
        Board(const Board& other);

        uint64_t bitboards[2][6];
        uint64_t occupiedSquares[2];
        GameState state;

        /**
         * @brief Makes a move and sets up game state for the next turn. Move must be pseudo-legal.
         * If the move was invalid, nothing will occur and INVALID_MOVE will be return
         * 
         * @param move the pseudo-legal move
         * @return UnmakeMove or INVALID_MOVE
         */
        UnmakeMove makeMove(const Move& move);
        void unmakeMove(const UnmakeMove& move);

        MoveList generatePLMoves() const;

        void addKingMoves(MoveList& moves) const;
        void addQueenMoves(MoveList& moves) const;
        void addKnightMoves(MoveList& moves) const;
        void addBishopMoves(MoveList& moves) const;
        void addRookMoves(MoveList& moves) const;
        void addPawnMoves(MoveList& moves) const;

        uint64_t plMoveBB(uint8_t pieceType, uint8_t square, uint8_t color) const;

        uint64_t plKingMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plQueenMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plBishopMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plKnightMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plRookMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plPawnMoveBB(uint8_t square, uint8_t color) const;

        uint64_t getAttacks(uint8_t color) const; // get attacks that a color is doing

        // very slow! use for convenience, not speed
        MateStatus getMateStatus() const;

        Board& operator=(const Board& other);

    private:
        inline void putPiece(uint8_t side, uint8_t piece, uint8_t index);
        inline void removePiece(uint8_t side, uint8_t piece, uint8_t index);
    };

    std::string indexToPrettyString(uint8_t index);
    std::string bitboardToPrettyString(uint64_t bitboard);
    std::string boardToPrettyString(const Board& board);
    std::string pieceToPrettyString(uint8_t piece);
} // namespace choco
