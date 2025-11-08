#pragma once

#include <cstdint>
#include <iostream>
#include <string>

namespace choco {
    class Board {
    public:
        Board();
        Board(const std::string& fen);

        size_t getPieceType(int rank, int file) const;
        size_t getPieceColor(int rank, int file) const;

        uint64_t bitboards[2][6];
    };

    uint64_t getMask(int rank, int file);
    int getIndex(int rank, int file);
} // namespace choco

std::ostream& operator<<(std::ostream& os, const choco::Board& obj);
