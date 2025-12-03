#include "search.h"

#include <limits>
#include <algorithm>
#include <random>
#include <string>
#include <iostream>
#include <cmath>
#include <limits>
#include <thread>
#include <atomic>
#include <cstring>
#include <algorithm>

#ifdef BOT_PERF_CTR
#include <chrono>
#endif

#include "macros.h"
#include "bithelpers.h"
#include "eval.h"
#include "uci.h"
#include "zobrist.h"
#include "movegen.h"

namespace choco {
    namespace {
#ifdef BOT_PERF_CTR
        int64_t getCurrentMs() {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        }
#endif

        inline float exchangeVal(Board& board, const Move& move) {
            uint8_t capturedPiece = getPieceOnSquare(board.bitboards[board.state.activeColor], move.to);
            return isValidPiece(capturedPiece) ? STATIC_PIECE_VALUES[capturedPiece] - STATIC_PIECE_VALUES[move.pieceType] : -100;
        }
    }

    static constexpr float MATE_EVAL = 32000;
    static constexpr float MATE_EVAL_THRESHOLD = 30000; // i can spot mate in 2000

    inline void Search::tt_store(uint64_t key, float eval, int depth, TTFlag flag, const Move& bestMove) {
        TTEntry& e = tt[key];
        if (e.key != key || depth >= e.depth) {
            e.key = key;
            e.eval = eval;
            e.depth = depth;
            e.flag = flag;
            e.bestMove = bestMove;
        }
    }

    inline bool Search::tt_lookup(uint64_t key, TTEntry& out, int requiredDepth) {
        TTEntry& e = tt[key];
        if (e.key == key && e.depth >= requiredDepth) {
            out = e;
            return true;
        }
        return false;
    }

    int64_t lastSearchMs = 0;
    int64_t nodesSearched = 0;
    int64_t lastSecondNS = 0;
    int64_t lastDepthNS = 0;

    inline float Search::quiesce(Board& board, float alpha, float beta) {
        if (!searching.load()) return std::numeric_limits<float>::quiet_NaN();

        // stand pat
        float stand = evaluate(board);
        if (stand >= beta) return stand;
        if (stand > alpha) alpha = stand;

        MoveList moves;
        getMoves<MoveType::NOISY>(board, moves);

        for (int i = 1; i < moves.size(); i++) {
            const Move& keyMove = moves[i];
            float key = exchangeVal(board, keyMove);
            float j = i - 1;

            while (j >= 0 && exchangeVal(board, moves[j]) < key) {
                moves[j + 1] = moves[j];
                j = j - 1;
            }
            moves[j + 1] = keyMove;
        }

        for (const Move& m : moves) {
            uint64_t toMask = getMask(m.to);

            UnmakeMove u = board.makeMove(m);
            if (!u.isValid()) continue;

            float score = -quiesce(board, -beta, -alpha);
            if (std::isnan(score)) return std::numeric_limits<float>::quiet_NaN();
            nodesSearched++;
            board.unmakeMove(u);

            if (score >= beta) return score;

            if (score > alpha) alpha = score;
        }

        return alpha;
    }

    float Search::negamax(Board& board, float alpha, float beta, int depth) {
        if (!searching.load(std::memory_order_acquire)) return std::numeric_limits<float>::quiet_NaN();
        
        if (depth <= 0) return quiesce(board, alpha, beta);

#ifdef BOT_PERF_CTR
        if (getCurrentMs() - lastSearchMs >= 1000) {
            lastSearchMs = getCurrentMs();
            std::cout << "info nps " << std::to_string(nodesSearched - lastSecondNS) << std::endl;
            lastSecondNS = nodesSearched;
            nodesSearched = 0;
        }
#endif // BOT_PERF_CTR

        uint64_t key = getHash(board);

        TTEntry entry;
        if (tt_lookup(key, entry, depth)) {
            if (entry.flag == TT_EXACT) return entry.eval;
            if (entry.flag == TT_ALPHA && entry.eval <= alpha) return alpha;
            if (entry.flag == TT_BETA  && entry.eval >= beta)  return beta;
        }

        float best = -MATE_EVAL;
        Move bestMove;

        MoveList moves;
        getMoves<MoveType::ALL>(board, moves);

        orderMoves(board, key, moves);
        
        bool invalidMove = true;

        int movesLooked = 0;

        for (const Move& m : moves) {
            UnmakeMove u = board.makeMove(m);
            if (!u.isValid()) continue;

            invalidMove = false;
            movesLooked++;
            
            // obsidian LMR formula
            int reductionDepth = depth <= 3 ? 1 : static_cast<int>(0.99 + std::log(depth) * std::log(movesLooked) / 3.14 + .5);
            // my LMR formula
            // int reductionDepth = 1;

            float score = -negamax(board, -beta, -alpha, depth - reductionDepth);
            board.unmakeMove(u);

            if (std::isnan(score)) return score;

            if (score > best) {
                best = score;
                bestMove = m;
            }
            if (score > alpha) alpha = score;

            if (score >= beta) {
                tt_store(key, best, depth, TTFlag::TT_BETA, m);
                nodesSearched++;
                return best;
            }
        }

        if (invalidMove) {
            // TEMPORARY MATE CHECK
            MoveList mateCheckList;
            getMoves<MoveType::NOISY>(board, mateCheckList);
            for (const Move& mateCheckMove : mateCheckList) {
                if (getMask(mateCheckMove.to) & board.bitboards[board.state.activeColor][KING]) {
                    return -MATE_EVAL;
                }
            }

            return 0; // stalemate
        }

        if (best > MATE_EVAL_THRESHOLD) {
            best--;
        } else if (best < -MATE_EVAL_THRESHOLD) {
            best++;
        }

        TTFlag flag = (best <= alpha ? TTFlag::TT_ALPHA : TTFlag::TT_EXACT);
        tt_store(key, best, depth, flag, bestMove);

        return best;
    }


    Search::Search(const Board& board) : board(board), searching(false),
            bestMove(bestMove = { INVALID_PIECE, INVALID_SQUARE, INVALID_SQUARE, INVALID_PIECE }),
            tt(TTEntry()) {}

    const Board& Search::getBoard() const {
        return board;
    }

    void Search::search(const SearchBounds& bound) {
        float bestEval = -9999999999999;
        searching.store(true);

        std::thread watcherThread([&]() -> void {
            std::this_thread::sleep_for(std::chrono::milliseconds(bound.moveTime));
            searching.store(false);
        });
        watcherThread.detach();

        int depthSoFar = 0;
        
        while (true) {
            float depthBestEval = -9999999999999;
            Move depthBestMove = { INVALID_PIECE, INVALID_SQUARE, INVALID_SQUARE, INVALID_PIECE };
            MoveList moves;
            getMoves<MoveType::ALL>(board, moves);

            for (const Move& move : moves) {
                UnmakeMove unmake = board.makeMove(move);
                if (!unmake.isValid()) continue;

                float eval = -negamax(board,
                                    -9999999999999,
                                    9999999999999,
                                    depthSoFar);
                if (std::isnan(eval) || !searching.load()) {
                    if (depthBestEval > bestEval) bestMove = depthBestMove;
                    std::cout << "bestmove " << moveToUci(bestMove) << std::endl;
                    return;
                }

                board.unmakeMove(unmake);

                if (eval > depthBestEval) {
                    depthBestEval = eval;
                    depthBestMove = move;
                }
            }

            bestEval = depthBestEval;
            bestMove = depthBestMove;

            std::cout << "info depth " << std::to_string(depthSoFar) << " ";

            if (std::abs(bestEval) >= MATE_EVAL_THRESHOLD) {
                int mateIn = MATE_EVAL - std::abs(bestEval);
                std::cout << "score mate " << std::to_string(mateIn) << " ";
            } else {
                std::cout << "score cp " << std::to_string((int)(bestEval * 100)) << " ";
            }
            std::cout << "pv " << moveToUci(bestMove) << std::endl;

            depthSoFar++;
        }
    }

    void Search::stop() {
        searching.store(false);
    }

    Move Search::getBestMove() {
        return bestMove;
    }

    void Search::playMove(const Move& move) {
        board.makeMove(move);
        bestMove = { INVALID_PIECE, INVALID_SQUARE, INVALID_SQUARE, INVALID_PIECE };
    }

    void Search::clearTT() {
        tt.reset(TTEntry());
    }

    inline void Search::orderMoves(Board& board, uint64_t boardHash, MoveList& moves) {
        // MOVE ORDERING
        // TTPV move
        TTEntry entry;
        bool lookupFound = false;
        if (tt_lookup(boardHash, entry, 0)) {
            for (uint8_t i = 0; i < moves.size(); i++) {
                if (entry.bestMove == moves[i]) {
                    moves.swap(0, i);
                    lookupFound = true;
                    break;
                }
            }
        }

        // sort
        for (int i = (lookupFound ? 1 : 2); i < moves.size(); i++) {
            const Move& keyMove = moves[i];
            uint8_t capturedPieceA = getPieceOnSquare(board.bitboards[board.state.activeColor], keyMove.to);
            float key = guessMoveOrderEval(board, keyMove);
            float j = i - 1;

            while (j >= 0 && guessMoveOrderEval(board, moves[j]) < key) {
                moves[j + 1] = moves[j];
                j = j - 1;
            }
            moves[j + 1] = keyMove;
        }
    }

    inline float Search::guessMoveOrderEval(Board& board, const Move& move) {
        return exchangeVal(board, move);
        // TTEntry entry;
        // return tt_lookup(getHash(board, move), entry, 0) ? entry.eval : exchangeVal(board, move);
    }
}
