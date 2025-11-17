#include "engine.h"

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

    static const float MATE_EVAL = 32000;

    enum TTFlag : uint8_t { TT_EXACT, TT_ALPHA, TT_BETA };

    struct TTEntry {
        uint64_t key = 0;
        float eval = 0;
        int depth = -1;
        uint8_t flag = TT_EXACT;
        Move bestMove;
    };

    constexpr int TT_BITS  = 22;
    constexpr int TT_SIZE  = 1 << TT_BITS;
    constexpr uint64_t TT_MASK = TT_SIZE - 1;

    static TTEntry TT[TT_SIZE];
    static TTEntry QTT[TT_SIZE];

    inline TTEntry& tt_probe(uint64_t key) {
        return TT[key & TT_MASK];
    }
    inline TTEntry& qtt_probe(uint64_t key) {
        return QTT[key & TT_MASK];
    }

    inline void tt_store(uint64_t key, float eval, int depth, TTFlag flag, const Move& bestMove) {
        TTEntry& e = tt_probe(key);
        if (e.key != key || depth >= e.depth) {
            e.key = key;
            e.eval = eval;
            e.depth = depth;
            e.flag = flag;
            e.bestMove = bestMove;
        }
    }

    inline bool tt_lookup(uint64_t key, TTEntry& out, int requiredDepth) {
        TTEntry& e = tt_probe(key);
        if (e.key == key && e.depth >= requiredDepth) {
            out = e;
            return true;
        }
        return false;
    }

    inline bool qtt_lookup(uint64_t key, float& out) {
        TTEntry& e = qtt_probe(key);
        if (e.key == key) {
            out = e.eval;
            return true;
        }
        return false;
    }

    inline void qtt_store(uint64_t key, float eval) {
        TTEntry& e = qtt_probe(key);
        e.key = key;
        e.eval = eval;
        e.depth = 0;
        e.flag = TT_EXACT;
    }

    static uint64_t TRANSPOSITION_HASHES[15][64] = {0};

    void initTT() {
        std::mt19937_64 engine(67);
        for (size_t i = 0; i < 15; i++) {
            for (size_t j = 0; j < 64; j++) {
                TRANSPOSITION_HASHES[i][j] = engine();
            }
        }
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

    uint64_t getHash(Board& board, const Move& nextMove) {
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

        UnmakeMove unmake = board.makeMove(nextMove);
        if (unmake.isValid()) {
            uint64_t hash = getHash(board);
            board.unmakeMove(unmake);
            return hash;
        }

        // undo getHash(Board&), since the en passant square has changed
        if (IS_VALID_SQUARE(board.state.enpassantSquare)) {
            hash ^= TRANSPOSITION_HASHES[14][getFile(board.state.enpassantSquare)];
        }

        // double push (en passant)
        if (nextMove.pieceType == PAWN && unsignedDist(nextMove.from, nextMove.to) == 16) {
            hash ^= TRANSPOSITION_HASHES[15][getFile(nextMove.from)];
        }

        return 0ULL;
    }

    static const float STATIC_PIECE_VALUES[6] = {
        1000, 9, 3.2, 3, 5, 1
    };

    static const float PIECE_SQUARE_TABLES[6][64] = {
        { 
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -20,-30,-30,-40,-40,-30,-30,-20,
            -10,-20,-20,-20,-20,-20,-20,-10,
            20, 20,  0,  0,  0,  0, 20, 20,
            20, 30, 10,  0,  0, 10, 30, 20
        },
        {
            -20,-10,-10, -5, -5,-10,-10,-20,
            -10,  0,  0,  0,  0,  0,  0,-10,
            -10,  0,  5,  5,  5,  5,  0,-10,
            -5,  0,  5,  5,  5,  5,  0, -5,
            0,  0,  5,  5,  5,  5,  0, -5,
            -10,  5,  5,  5,  5,  5,  0,-10,
            -10,  0,  5,  0,  0,  0,  0,-10,
            -20,-10,-10, -5, -5,-10,-10,-20
        },
        {
            -20,-10,-10,-10,-10,-10,-10,-20,
            -10,  0,  0,  0,  0,  0,  0,-10,
            -10,  0,  5, 10, 10,  5,  0,-10,
            -10,  5,  5, 10, 10,  5,  5,-10,
            -10,  0, 10, 10, 10, 10,  0,-10,
            -10, 10, 10, 10, 10, 10, 10,-10,
            -10,  5,  0,  0,  0,  0,  5,-10,
            -20,-10,-10,-10,-10,-10,-10,-20,
        },
        {
            -50,-40,-30,-30,-30,-30,-40,-50,
            -40,-20,  0,  0,  0,  0,-20,-40,
            -30,  0, 10, 15, 15, 10,  0,-30,
            -30,  5, 15, 20, 20, 15,  5,-30,
            -30,  0, 15, 20, 20, 15,  0,-30,
            -30,  5, 10, 15, 15, 10,  5,-30,
            -40,-20,  0,  5,  5,  0,-20,-40,
            -50,-40,-30,-30,-30,-30,-40,-50,
        },
        {
            0,  0,  0,  0,  0,  0,  0,  0,
            5, 10, 10, 10, 10, 10, 10,  5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            0,  0,  0,  5,  5,  0,  0,  0
        },
        {
            0,  0,  0,  0,  0,  0,  0,  0,
            50, 50, 50, 50, 50, 50, 50, 50,
            10, 10, 20, 30, 30, 20, 10, 10,
            5,  5, 10, 25, 25, 10,  5,  5,
            0,  0,  0, 20, 20,  0,  0,  0,
            5, -5,-10, 0, 0,-10, -5,  5,
            5, 10, 10,-20,-20, 10, 10,  5,
            0,  0,  0,  0,  0,  0,  0,  0
        }
    };

    float staticEvaluate(const Board& board) {
        float eval = 0;
        uint8_t activeColor = board.state.activeColor;
        uint8_t opp = OPPOSITE_SIDE(activeColor);

        // material
        for (int i = 1; i < 6; i++) {
            eval += STATIC_PIECE_VALUES[i] * board.countPieces(activeColor, i);
            eval -= STATIC_PIECE_VALUES[i] * board.countPieces(opp, i);
        }

        float mobility = 0;
        float pst = 0;

        for (int p = 1; p < 6; p++) {
            iterateIndices(board.bitboards[activeColor][p],
                [p, &mobility, &board, &pst, activeColor](uint8_t idx){
                    mobility += countOnes(board.plMoveBB(p, idx, activeColor));
                    size_t pstIdx = (board.state.activeColor == SIDE_WHITE) ? 63 - p : p;
                    pst += PIECE_SQUARE_TABLES[p][pstIdx];
                });

            iterateIndices(board.bitboards[opp][p],
                [p, &mobility, &board, &pst, opp](uint8_t idx){
                    mobility -= countOnes(board.plMoveBB(p, idx, opp));
                    size_t pstIdx = (board.state.activeColor == SIDE_WHITE) ? p : 63 - p;
                    pst -= PIECE_SQUARE_TABLES[p][pstIdx];
                });
        }

        return eval + mobility * 0.1f + pst * 0.001f;
    }

    int pruned = 0;

    float quiesce(Board& board, float alpha, float beta) {
        float stand = staticEvaluate(board);
        if (stand >= beta) return stand;
        if (stand > alpha) alpha = stand;

        uint64_t key = getHash(board);

        float cached;
        if (qtt_lookup(key, cached))
            return cached;

        std::vector<Move> moves = board.generatePLMoves();
        uint64_t oppPieces = getOccupiedBitboard(board.bitboards[OPPOSITE_SIDE(board.state.activeColor)]);

        for (const Move& m : moves) {
            uint64_t toMask = getMask(m.to);

            bool capture = toMask & oppPieces;
            bool promo   = IS_VALID_PIECE(m.promotionType);

            if (!capture && !promo)
                continue;

            UnmakeMove u = board.makeMove(m);
            if (!u.isValid()) continue;

            float score = -quiesce(board, -beta, -alpha);
            board.unmakeMove(u);

            if (score >= beta) {
                qtt_store(key, score);
                pruned++;
                return score;
            }
            if (score > alpha) {
                alpha = score;
            }
        }

        qtt_store(key, alpha);
        return alpha;
    }

    float evaluate(Board& board, float alpha, float beta, int depth) {
        if (depth == 0) return quiesce(board, alpha, beta);

        uint64_t key = getHash(board);

        TTEntry entry;
        if (tt_lookup(key, entry, depth)) {
            if (entry.flag == TT_EXACT) return entry.eval;
            if (entry.flag == TT_ALPHA && entry.eval <= alpha) return alpha;
            if (entry.flag == TT_BETA  && entry.eval >= beta)  return beta;
        }

        float best = -MATE_EVAL;
        Move bestMove;

        std::vector<Move> moves = board.generatePLMoves();

        if (tt_lookup(key, entry, 0)) {
            auto it = std::find(moves.begin(), moves.end(), entry.bestMove);
            if (it != moves.end()) {
                std::rotate(moves.begin(), it, it + 1);
            }
        }

        for (const Move& m : moves) {
            UnmakeMove u = board.makeMove(m);
            if (!u.isValid()) continue;

            float score = -evaluate(board, -beta, -alpha, depth - 1);
            board.unmakeMove(u);

            if (score > best) {
                best = score;
                bestMove = m;
            }
            if (score > alpha) alpha = score;

            if (score >= beta) {
                tt_store(key, best, depth, TT_BETA, m);
                pruned++;
                return best;
            }
        }

        TTFlag flag = (best <= alpha ? TT_ALPHA : TT_EXACT);
        tt_store(key, best, depth, flag, bestMove);

        return best;
    }


    EvalNode::EvalNode(Board& board, float e)
        : board(board), eval(e) {}

    Engine::Engine(Board& board) {
        head = new EvalNode(board, 0);
    }

    Move Engine::getBestMove(uint16_t depth) {
        Move bestMove(0, 0, 0);
        float bestEval = -std::numeric_limits<float>::infinity();

        for (int d = 1; d <= depth; d++) { // "iterative deepening"
            std::cout << "Searching " << std::to_string(d) << "-ply:" << std::endl;
            std::vector<Move> moves = head->board.generatePLMoves();
            std::cout << std::to_string(moves.size()) << std::endl;

            for (const Move& move : moves) {
                UnmakeMove unmake = head->board.makeMove(move);
                if (!unmake.isValid()) continue;
                std::cout << "  > Attempting " << indexToPrettyString(move.from)
                        << " to " << indexToPrettyString(move.to) << " - ";

                float eval = -evaluate(head->board,
                                    -std::numeric_limits<float>::infinity(),
                                    std::numeric_limits<float>::infinity(),
                                    d);
                std::cout << std::to_string(eval) << std::endl;
                head->board.unmakeMove(unmake);

                if (eval > bestEval) {
                    bestEval = eval;
                    bestMove = move;
                }
            }
        }

        return bestMove;
    }

}
