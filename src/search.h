#pragma once

#include "macros.h"
#include "board.h"

namespace choco {
    void initTT(); // should be called before any engine stuff
    
    enum TTFlag : uint8_t { TT_EXACT, TT_ALPHA, TT_BETA };

    struct TTEntry {
        uint64_t key = 0;
        float eval = 0;
        int depth = -1;
        TTFlag flag = TT_EXACT;
        Move bestMove;
    };

    class Search {
    public:
        Search(const Board& board);

        Move getBestMove(uint16_t depth);
    private:
        Board board;

        static constexpr int TT_BITS  = 22;
        static constexpr size_t TT_SIZE  = 1 << TT_BITS;
        static constexpr uint64_t TT_MASK = TT_SIZE - 1;

        TTEntry *TT;
        TTEntry *QTT;

        inline TTEntry& tt_probe(uint64_t key);
        inline TTEntry& qtt_probe(uint64_t key);
        inline void tt_store(uint64_t key, float eval, int depth, TTFlag flag, const Move& bestMove);
        inline bool tt_lookup(uint64_t key, TTEntry& out, int requiredDepth);
        inline bool qtt_lookup(uint64_t key, float& out);
        inline void qtt_store(uint64_t key, float eval);

        float quiesce(Board& board, float alpha, float beta);
        float negamax(Board& board, float alpha, float beta, int depth);
    };
} // namespace choco
