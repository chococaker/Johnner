#pragma once

#include "board.h"

namespace choco {
    void perft(Board& board,
               int depth,
               bool initBB,
               bool print);
} // namespace choco
