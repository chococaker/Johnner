#pragma once

#include "tree.h"
#include "board.h"

namespace choco {
    class EvalPosition {
        Board board;
        float rating;
    };

    typedef Node<EvalPosition> EvalNode;

    class Engine {
        EvalNode* head;
        Move getBestMove(uint16_t depth) const;
        void playMove(const Move& move) const;
    };
} // namespace choco
