#pragma once

#include "board.h"

namespace choco {
    uint32_t perft(Board& board,
               int depth,
               bool initBB,
               bool print);
} // namespace choco
