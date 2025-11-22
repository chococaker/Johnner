#include "search.h"

#include <limits>
#include <algorithm>
#include <random>
#include <string>
#include <iostream>

#ifdef BOT_PERF_CTR
#include <chrono>
#endif

#include "macros.h"
#include "bithelpers.h"
#include "eval.h"

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
        std::mt19937_64 engine(69);
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
    inline TTEntry& Search::qtt_probe(uint64_t key) {
        return QTT[key & TT_MASK];
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

    inline bool Search::qtt_lookup(uint64_t key, float& out) {
        TTEntry& e = qtt_probe(key);
        if (e.key == key) {
            out = e.eval;
            return true;
        }
        return false;
    }

    inline void Search::qtt_store(uint64_t key, float eval) {
        TTEntry& e = qtt_probe(key);
        e.key = key;
        e.eval = eval;
        e.depth = 0;
        e.flag = TT_EXACT;
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

    inline float Search::quiesce(Board& board, float alpha, float beta) {
        float stand = evaluate(board);
        if (stand >= beta) return stand;
        if (stand > alpha) alpha = stand;

        uint64_t key = getHash(board);

        float cached;
        if (qtt_lookup(key, cached))
            return cached;

        MoveList moves = board.generatePLMoves();
        uint64_t oppPieces = board.occupiedSquares[OPPOSITE_SIDE(board.state.activeColor)];

        for (const Move& m : moves) {
            uint64_t toMask = getMask(m.to);

            bool capture = toMask & oppPieces;
            bool promo   = IS_VALID_PIECE(m.promotionType);

            if (!capture && !promo)
                continue;

            UnmakeMove u = board.makeMove(m);
            if (!u.isValid()) continue;

            float score = -quiesce(board, -beta, -alpha);
            nodes++;
            board.unmakeMove(u);

            if (score >= beta) {
                qtt_store(key, score);
                return score;
            }
            if (score > alpha) {
                alpha = score;
            }
        }

        qtt_store(key, alpha);
        return alpha;
    }
    
    int64_t lastAnalysisMs = 0;

    float Search::negamax(Board& board, float alpha, float beta, int depth) {
        if (depth <= 0) return quiesce(board, alpha, beta);

        if (getCurrentMs() - lastAnalysisMs > 1000) {
            lastAnalysisMs = getCurrentMs();
            std::cout << std::to_string(nodes) << " n/s" << std::endl;
            nodes = 0;
        }

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

        for (const Move& m : moves) {
            UnmakeMove u = board.makeMove(m);
            if (!u.isValid()) continue;

            float score;
            
            score = -negamax(board, -beta, -alpha, depth - 1);

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

        TTFlag flag = (best <= alpha ? TTFlag::TT_ALPHA : TTFlag::TT_EXACT);
        tt_store(key, best, depth, flag, bestMove);

        if (best > MATE_EVAL_THRESHOLD) {
            best--;
        } else if (best < -MATE_EVAL_THRESHOLD) {
            best++;
        }

        return best;
    }


    Search::Search(const Board& board) : board(board) {
        TT = new TTEntry[TT_SIZE];
        QTT = new TTEntry[TT_SIZE];
    }

    Move Search::getBestMove(uint16_t depth) {
        Move bestMove(0, 0, 0);
        float bestEval = -std::numeric_limits<float>::infinity();

        for (int d = 2; d <= depth; d++) { // "iterative deepening"
            std::cout << "Searching " << std::to_string(d) << "-ply (";
            MoveList moves = board.generatePLMoves();
            std::cout << std::to_string(moves.size()) << " PL moves)" << std::endl;

            for (const Move& move : moves) {
                UnmakeMove unmake = board.makeMove(move);
                if (!unmake.isValid()) continue;
                std::cout << "  > Attempting " << indexToPrettyString(move.from)
                        << " to " << indexToPrettyString(move.to) << " - ";

                float eval = -negamax(board,
                                    -std::numeric_limits<float>::infinity(),
                                    std::numeric_limits<float>::infinity(),
                                    d);
                std::cout << std::to_string(eval) << std::endl;
                board.unmakeMove(unmake);

                if (eval > bestEval) {
                    bestEval = eval;
                    bestMove = move;
                }
            }

            std::cout << std::to_string(d) << "-ply best move: "
                      << choco::pieceToPrettyString(bestMove.pieceType)
                      << " "
                      << choco::indexToPrettyString(bestMove.from)
                      << " to " << choco::indexToPrettyString(bestMove.to)
                      << std::endl;
        }

        return bestMove;
    }

    inline float exchangeVal(Board& board, const Move& move) {
        uint8_t capturedPiece = getPieceOnSquare(board.bitboards[board.state.activeColor], move.to);
        return (capturedPiece == INVALID_PIECE) ? 0 : STATIC_PIECE_VALUES[move.pieceType];
    }

    inline void Search::orderMoves(Board& board, uint64_t boardHash, MoveList& moves) {
        TTEntry entry;

        // MOVE ORDERING
        // TTPV move
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
        for (int i = (lookupFound ? 0 : 1); i < moves.size(); i++) {
            const Move& keyMove = moves[i];
            uint8_t capturedPieceA = getPieceOnSquare(board.bitboards[board.state.activeColor], keyMove.to);
            float key = exchangeVal(board, keyMove);
            float j = i - 1;

            while (j >= 0 && exchangeVal(board, moves[j]) > key) {
                moves[j + 1] = moves[j];
                j = j - 1;
            }
            moves[j + 1] = keyMove;
        }
    }

    Search::~Search() {
        delete[] TT;
        delete[] QTT;
    }
}
