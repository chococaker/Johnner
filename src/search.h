#pragma once

#include "macros.h"
#include "board.h"
#include "types.h"

#include <atomic>
#include <thread>

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

    struct SearchBounds {
        int64_t moveTime;
    };

    class Search {
    public:
        Search(const Board& board);

        void search(const SearchBounds& bound);
        void stop();

        Move getBestMove();

        const Board& getBoard() const;

        void playMove(const Move& move);

        template<bool clearTT>
        void setBoard(const Board& board);
        void clearTT();

        ~Search();
    private:
        Board board;
        Move bestMove;

        int depthSoFar;
        std::atomic<bool> searching;

        static constexpr int TT_BITS  = 22;
        static constexpr size_t TT_SIZE  = 1 << TT_BITS;
        static constexpr uint64_t TT_MASK = TT_SIZE - 1;

        TTEntry *TT;

        inline TTEntry& tt_probe(uint64_t key);
        inline void tt_store(uint64_t key, float eval, int depth, TTFlag flag, const Move& bestMove);
        inline bool tt_lookup(uint64_t key, TTEntry& out, int requiredDepth);

        inline float quiesce(Board& board, float alpha, float beta);
        float negamax(Board& board, float alpha, float beta, int depth);

        inline void orderMoves(Board& board, uint64_t boardHash, MoveList& moves);
    };
    

    template<bool clearTT>
    void Search::setBoard(const Board& board) {
        this->board = board;
        if constexpr (clearTT) {
            this->clearTT();
        }
    }
} // namespace choco
