#pragma once

#include "types.h"
#include "board.h"

namespace choco {
    void initMoveGen();

    enum class MoveType {
        NOISY, QUIET, ALL
    };

    template<MoveType moveType>
    void getMoves(const Board& board, MoveList& moveList);
} // namespace choco
