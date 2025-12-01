#pragma once

#include <cstdint>
#include <random>
#include <algorithm>

#include "macros.h"
#include "board.h"
#include "bithelpers.h"

namespace choco {
    static uint64_t ZOBRIST_HASHES[15][64];

    static constexpr int TT_BITS  = 22;
    static constexpr size_t TT_SIZE  = 1 << TT_BITS;
    static constexpr uint64_t TT_MASK = TT_SIZE - 1;

    void initZobrist();

    inline uint64_t getHash(const Board& board) {
        uint64_t hash = 0;

        for (int color = 0; color < 2; color++) {
            for (int p = 0; p < 6; p++) {
                uint64_t bb = board.bitboards[color][p];
                while (bb) {
                    uint8_t idx = countTrailingZeros(bb);
                    bb &= bb - 1;
                    int hashIdx = p + (color * 6);
                    hash ^= ZOBRIST_HASHES[hashIdx][idx];
                }
            }
        }

        hash ^= ZOBRIST_HASHES[12][board.state.activeColor];

        if (board.state.canCastle(SIDE_WHITE, KING))  hash ^= ZOBRIST_HASHES[13][0];
        if (board.state.canCastle(SIDE_WHITE, QUEEN)) hash ^= ZOBRIST_HASHES[13][1];
        if (board.state.canCastle(SIDE_BLACK, KING))  hash ^= ZOBRIST_HASHES[13][2];
        if (board.state.canCastle(SIDE_BLACK, QUEEN)) hash ^= ZOBRIST_HASHES[13][3];

        if (IS_VALID_SQUARE(board.state.enpassantSquare)) {
            hash ^= ZOBRIST_HASHES[14][getFile(board.state.enpassantSquare)];
        }

        return hash & TT_MASK;
    }
    inline uint64_t getHash(const Board& board, const Move& nextMove) {
        uint64_t hash = getHash(board);

        hash ^= ZOBRIST_HASHES[nextMove.pieceType][nextMove.from];
        hash ^= ZOBRIST_HASHES[nextMove.pieceType][nextMove.to];

        uint8_t capturedPiece = getPieceOnSquare(
                                board.bitboards[OPPOSITE_SIDE(board.state.activeColor)],
                                nextMove.to);
        if (capturedPiece != INVALID_PIECE) {
            hash ^= ZOBRIST_HASHES[capturedPiece][nextMove.to];
        }

        hash ^= ZOBRIST_HASHES[12][board.state.activeColor];
        hash ^= ZOBRIST_HASHES[12][OPPOSITE_SIDE(board.state.activeColor)];

        if (nextMove.pieceType == KING && (nextMove.from == E1 || nextMove.from == E8)
            && (board.state.canCastle(board.state.activeColor, KING)
                || board.state.canCastle(board.state.activeColor, KING))) { // disable castling
            hash ^= ZOBRIST_HASHES[13][0 + 2 * board.state.activeColor];
            hash ^= ZOBRIST_HASHES[13][1 + 2 * board.state.activeColor];
        }

        if ((nextMove.from == A1 || nextMove.to == A1) && board.state.canCastle(SIDE_WHITE, KING))
            hash ^= ZOBRIST_HASHES[13][0];
        if ((nextMove.from == H1 || nextMove.to == H1) && board.state.canCastle(SIDE_WHITE, QUEEN))
            hash ^= ZOBRIST_HASHES[13][1];
        if ((nextMove.from == A8 || nextMove.to == A8) && board.state.canCastle(SIDE_BLACK, KING))
            hash ^= ZOBRIST_HASHES[13][2];
        if ((nextMove.from == H8 || nextMove.to == H8) && board.state.canCastle(SIDE_BLACK, QUEEN))
            hash ^= ZOBRIST_HASHES[13][3];
        
        // undo getHash(Board&), since the en passant square has changed
        if (IS_VALID_SQUARE(board.state.enpassantSquare)) {
            hash ^= ZOBRIST_HASHES[14][getFile(board.state.enpassantSquare)];
        }

        // double push (en passant)
        if (nextMove.pieceType == PAWN && unsignedDist(nextMove.from, nextMove.to) == 16) {
            hash ^= ZOBRIST_HASHES[15][getFile(nextMove.from)];
        }

        return hash & TT_MASK;
    }

    template<typename T>
    class ZobristTable {
    public:
        ZobristTable(const T& fillValue) {
            table = new T[TT_SIZE];
            reset(fillValue);
        }

        ZobristTable(const ZobristTable& other) {
            table = new T[TT_SIZE];
            std::copy(other.table, other.table + TT_SIZE, table);
        }

        inline T& operator[](Board& board) {
            return table[getHash(board)];
        }

        inline const T& operator[](Board& board) const {
            return table[getHash(board)];
        }

        inline T& operator[](uint64_t hash) {
            return table[hash];
        }

        inline const T& operator[](uint64_t hash) const {
            return hash;
        }

        inline void reset(const T& fillValue) {
            std::fill(table, table + TT_SIZE, fillValue);
        }

        ~ZobristTable() {
            delete[] table;
        }

    private:
        T* table;
    };
} // namespace choco
