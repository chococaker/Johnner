#pragma once

#include <cstdint>
#include <iostream>

namespace choco {
    enum class PieceType {
        KING = 0,
        QUEEN = 1,
        BISHOP = 2,
        KNIGHT = 3,
        ROOK = 4,
        PAWN = 5
    };

    enum class PieceColor {
        SIDE_WHITE = 0, SIDE_BLACK = 1
    };

    class Board {
    public:
        PieceType getPieceType(int row, int col) const;
        PieceColor getPieceColor(int row, int col) const;

        uint64_t w_bitboard[6];
        uint64_t b_bitboard[6];
    };
} // namespace choco

std::ostream& operator<<(std::ostream& os, const choco::Board& obj);
