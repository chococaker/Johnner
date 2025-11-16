#pragma once

#include "macros.h"
#include "board.h"

namespace choco {
    void initTT(); // should be called before any engine stuff
    
    struct EvalNode {
        EvalNode(Board& board, float eval);

        Board board;
        float eval;
        std::vector<EvalNode*> children;
    };

    class Engine {
    public:
        Engine(Board& board);

        EvalNode* head;
        Move getBestMove(uint16_t depth);
        void playMove(const Move& move) const;
    };
} // namespace choco
