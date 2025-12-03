#pragma once

#include "board.h"

namespace choco {
    uint32_t perft(Board& board,
               int depth,
               bool print);
} // namespace choco
