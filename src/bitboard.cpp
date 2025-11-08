#include "bitboard.h"
#include <iostream>

namespace choco {
    PieceType Board::getPieceType(int row, int col) const {
        return PieceType::BISHOP;
    }

    PieceColor Board::getPieceColor(int row, int col) const {
        return PieceColor::SIDE_WHITE;
    }
    
} // namespace choco

std::ostream& operator<<(std::ostream& os, const choco::Board& obj) {
    for (size_t i = 0; i < 64; i++) {
        os << ((obj.w_bitboard[static_cast<int>(choco::PieceType::KING)] >> i) & 1);
        os << " ";
        if (i % 8 == 7) os << std::endl;
    }
    return os;
}
