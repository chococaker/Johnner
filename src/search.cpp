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

namespace choco {
    namespace {
#ifdef BOT_PERF_CTR
        int64_t getCurrentMs() {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        }
#endif
    }

    static uint64_t TRANSPOSITION_HASHES[15][64] = {0};

    void initTT() {
        std::mt19937_64 engine(670);
        for (size_t i = 0; i < 15; i++) {
            for (size_t j = 0; j < 64; j++) {
                TRANSPOSITION_HASHES[i][j] = engine();
            }
        }
    }

    static constexpr float MATE_EVAL = 32000;
    static constexpr float MATE_EVAL_THRESHOLD = 30000;

    inline TTEntry& Search::tt_probe(uint64_t key) {
        return TT[key & TT_MASK];
    }

    inline void Search::tt_store(uint64_t key, float eval, int depth, TTFlag flag, const Move& bestMove) {
        TTEntry& e = tt_probe(key);
        if (e.key != key || depth >= e.depth) {
            e.key = key;
            e.eval = eval;
            e.depth = depth;
            e.flag = flag;
            e.bestMove = bestMove;
        }
    }

    inline bool Search::tt_lookup(uint64_t key, TTEntry& out, int requiredDepth) {
        TTEntry& e = tt_probe(key);
        if (e.key == key && e.depth >= requiredDepth) {
            out = e;
            return true;
        }
        return false;
    }

    uint64_t getHash(const Board& board) {
        uint64_t hash = 0;

        for (int color = 0; color < 2; color++) {
            for (int p = 0; p < 6; p++) {
                uint64_t bb = board.bitboards[color][p];
                while (bb) {
                    uint8_t idx = countTrailingZeros(bb);
                    bb &= bb - 1;
                    int hashIdx = p + (color * 6);
                    hash ^= TRANSPOSITION_HASHES[hashIdx][idx];
                }
            }
        }

        hash ^= TRANSPOSITION_HASHES[12][board.state.activeColor];

        if (board.state.canCastle(SIDE_WHITE, KING))  hash ^= TRANSPOSITION_HASHES[13][0];
        if (board.state.canCastle(SIDE_WHITE, QUEEN)) hash ^= TRANSPOSITION_HASHES[13][1];
        if (board.state.canCastle(SIDE_BLACK, KING))  hash ^= TRANSPOSITION_HASHES[13][2];
        if (board.state.canCastle(SIDE_BLACK, QUEEN)) hash ^= TRANSPOSITION_HASHES[13][3];

        if (IS_VALID_SQUARE(board.state.enpassantSquare)) {
            hash ^= TRANSPOSITION_HASHES[14][getFile(board.state.enpassantSquare)];
        }

        return hash;
    }

    uint64_t getHash(const Board& board, const Move& nextMove) {
        uint64_t hash = getHash(board);

        hash ^= TRANSPOSITION_HASHES[nextMove.pieceType][nextMove.from];
        hash ^= TRANSPOSITION_HASHES[nextMove.pieceType][nextMove.to];

        uint8_t capturedPiece = getPieceOnSquare(
                                board.bitboards[OPPOSITE_SIDE(board.state.activeColor)],
                                nextMove.to);
        if (capturedPiece != INVALID_PIECE) {
            hash ^= TRANSPOSITION_HASHES[capturedPiece][nextMove.to];
        }

        hash ^= TRANSPOSITION_HASHES[12][board.state.activeColor];
        hash ^= TRANSPOSITION_HASHES[12][OPPOSITE_SIDE(board.state.activeColor)];

        if (nextMove.pieceType == KING && (nextMove.from == E1 || nextMove.from == E8)
            && (board.state.canCastle(board.state.activeColor, KING)
                || board.state.canCastle(board.state.activeColor, KING))) { // disable castling
            hash ^= TRANSPOSITION_HASHES[13][0 + 2 * board.state.activeColor];
            hash ^= TRANSPOSITION_HASHES[13][1 + 2 * board.state.activeColor];
        }

        if ((nextMove.from == A1 || nextMove.to == A1) && board.state.canCastle(SIDE_WHITE, KING))
            hash ^= TRANSPOSITION_HASHES[13][0];
        if ((nextMove.from == H1 || nextMove.to == H1) && board.state.canCastle(SIDE_WHITE, QUEEN))
            hash ^= TRANSPOSITION_HASHES[13][1];
        if ((nextMove.from == A8 || nextMove.to == A8) && board.state.canCastle(SIDE_BLACK, KING))
            hash ^= TRANSPOSITION_HASHES[13][2];
        if ((nextMove.from == H8 || nextMove.to == H8) && board.state.canCastle(SIDE_BLACK, QUEEN))
            hash ^= TRANSPOSITION_HASHES[13][3];
        
        // undo getHash(Board&), since the en passant square has changed
        if (IS_VALID_SQUARE(board.state.enpassantSquare)) {
            hash ^= TRANSPOSITION_HASHES[14][getFile(board.state.enpassantSquare)];
        }

        // double push (en passant)
        if (nextMove.pieceType == PAWN && unsignedDist(nextMove.from, nextMove.to) == 16) {
            hash ^= TRANSPOSITION_HASHES[15][getFile(nextMove.from)];
        }

        return hash;
    }

    int nodes = 0;

    inline float exchangeVal(Board& board, const Move& move) {
        uint8_t capturedPiece = getPieceOnSquare(board.bitboards[board.state.activeColor], move.to);
        return isValidPiece(capturedPiece) ? STATIC_PIECE_VALUES[capturedPiece] - STATIC_PIECE_VALUES[move.pieceType] : 0;
    }

    inline float Search::quiesce(Board& board, float alpha, float beta) {
        if (!searching.load()) return std::numeric_limits<float>::quiet_NaN();

        float stand = evaluate(board);
        if (stand >= beta) return stand;
        if (stand > alpha) alpha = stand;

        MoveList moves = board.generatePLMoves();
        uint64_t oppPieces = board.occupiedSquares[OPPOSITE_SIDE(board.state.activeColor)];

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

            bool capture = toMask & oppPieces;
            bool promo   = IS_VALID_PIECE(m.promotionType);

            if (!capture && !promo)
                continue;

            UnmakeMove u = board.makeMove(m);
            if (!u.isValid()) continue;

            float score = -quiesce(board, -beta, -alpha);
            if (std::isnan(score)) return std::numeric_limits<float>::quiet_NaN();
            nodes++;
            board.unmakeMove(u);

            if (score >= beta) return score;

            // delta pruning
            float delta = STATIC_PIECE_VALUES[QUEEN];
            if (isValidPiece(m.promotionType)) delta *= 2;
            if (score < (alpha - delta)) {
                return alpha;
            }

            if (score > alpha) alpha = score;
        }

        return alpha;
    }

#ifdef BOT_PERF_CTR
    int64_t lastAnalysisMs = 0;
#endif // BOT_PERF_CTR

    float Search::negamax(Board& board, float alpha, float beta, int depth) {
        if (!searching.load(std::memory_order_acquire)) return std::numeric_limits<double>::quiet_NaN();
        
        if (depth <= 0) return quiesce(board, alpha, beta);

#ifdef BOT_PERF_CTR
        if (getCurrentMs() - lastAnalysisMs > 1000) {
            lastAnalysisMs = getCurrentMs();
            std::cout << "info nps " << std::to_string(nodes) << std::endl;
            nodes = 0;
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

        MoveList moves = board.generatePLMoves();

        orderMoves(board, key, moves);
        
        bool invalidMove = true;

        int movesLooked = 0;
        // obsidian lmr formula
        const int lmrCutoff = (int)(0.99 + std::log(depth) * std::log(moves.size()) / 3.14);

        for (const Move& m : moves) {
            bool shouldReduce = (movesLooked++ >= lmrCutoff && depth > 2);

            UnmakeMove u = board.makeMove(m);
            if (!u.isValid()) continue;

            invalidMove = false;
            
            float score = -negamax(board, -beta, -alpha, depth - 1 - 1 * shouldReduce);

            if (std::isnan(score)) return score;

            board.unmakeMove(u);

            if (score > best) {
                best = score;
                bestMove = m;
            }
            if (score > alpha) alpha = score;

            if (score >= beta) {
                tt_store(key, best, depth, TTFlag::TT_BETA, m);
                nodes++;
                return best;
            }
        }

        if (invalidMove) {
            if (board.bitboards[board.state.activeColor][KING]
                & board.getAttacks(OPPOSITE_SIDE(board.state.activeColor))) {
                    return -MATE_EVAL;
            } else {
                return 0;
            }
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


    Search::Search(const Board& board) : board(board), depthSoFar(0), searching(false),
            bestMove(bestMove = { INVALID_PIECE, INVALID_SQUARE, INVALID_SQUARE, INVALID_PIECE }) {
        TT = new TTEntry[TT_SIZE];
        clearTT();
    }

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

        
        while (true) {
            depthSoFar++;

            float depthBestEval = -9999999999999;
            Move depthBestMove = { INVALID_PIECE, INVALID_SQUARE, INVALID_SQUARE, INVALID_PIECE };
            MoveList moves = board.generatePLMoves();

            for (const Move& move : moves) {
                UnmakeMove unmake = board.makeMove(move);
                if (!unmake.isValid()) continue;

                float eval = -negamax(board,
                                      -9999999999999,
                                      9999999999999,
                                      depthSoFar);
                if (std::isnan(eval) || !searching) {
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
                std::cout << "score mate " << std::to_string(mateIn / 2 + 1) << " ";
            } else {
                std::cout << "score cp " << std::to_string((int)(bestEval * 100)) << " ";
            }
            std::cout << "pv " << moveToUci(bestMove) << std::endl;
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
        depthSoFar = 0;
    }

    void Search::clearTT() {
        std::fill(TT, TT + TT_SIZE, TTEntry());
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

        // insertion sort
        // MVV/LVA
        for (int i = (lookupFound ? 1 : 2); i < moves.size(); i++) {
            const Move& keyMove = moves[i];
            float key = exchangeVal(board, keyMove);
            int j = i - 1;

            while (j >= 0 && exchangeVal(board, moves[j]) > key) {
                moves[j + 1] = moves[j];
                j = j - 1;
            }
            moves[j + 1] = keyMove;
        }
    }

    Search::~Search() {
        delete[] TT;
    }
}
