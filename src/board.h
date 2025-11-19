#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <functional>

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

    class Move {
    public:
        Move() { }

        Move(uint8_t pieceType, uint8_t from, uint8_t to)
                : pieceType(pieceType), from(from), to(to), promotionType(6) { }

        Move(uint8_t pieceType, uint8_t from, uint8_t to, uint8_t promotionType)
                : pieceType(pieceType), from(from), to(to), promotionType(promotionType) { }

        uint8_t pieceType;
        uint8_t from;
        uint8_t to;
        uint8_t promotionType;

        bool operator==(const Move& other);
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


    class Board {
    public:
        Board();
        Board(std::string fen);

        uint64_t bitboards[2][6];
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

        std::vector<Move> generatePLMoves() const;

        void addKingMoves(std::vector<Move>& moves) const;
        void addQueenMoves(std::vector<Move>& moves) const;
        void addKnightMoves(std::vector<Move>& moves) const;
        void addBishopMoves(std::vector<Move>& moves) const;
        void addRookMoves(std::vector<Move>& moves) const;
        void addPawnMoves(std::vector<Move>& moves) const;

        uint8_t countPieces(uint8_t side, uint8_t piece) const;

        uint64_t plMoveBB(uint8_t pieceType, uint8_t square, uint8_t color) const;

        uint64_t plKingMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plQueenMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plBishopMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plKnightMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plRookMoveBB(uint8_t square, uint8_t color) const;
        uint64_t plPawnMoveBB(uint8_t square, uint8_t color) const;

        uint64_t getAttacks(uint8_t color) const; // get attacks that a color is doing

    private:
        void putPiece(uint8_t side, uint8_t piece, uint8_t index);
        void removePiece(uint8_t side, uint8_t piece, uint8_t index);
    };

    std::string indexToPrettyString(uint8_t index);
    std::string bitboardToPrettyString(uint64_t bitboard);
    std::string boardToPrettyString(const Board& board);
    std::string pieceToPrettyString(uint8_t piece);
} // namespace choco
