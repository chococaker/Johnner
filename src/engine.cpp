#include "engine.h"

#include <limits>
#include <algorithm>
#include <random>
#include <unordered_map>

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

            return std::chrono::duration_cast<std::chrono::milliseconds>(
                    duration)
                    .count();
        }
#endif
    }

    static const float MATE_EVAL = 32000;
    static const float MATE_EVAL_THRESHOLD = MATE_EVAL - 500; // I can spot mate in 500, technically!
    
    struct TTEntry {
        int depth;
        float eval;
    };

    /*
     * 0-  white king
     * 1-  white queen
     * 2-  white bishop
     * 3-  white knight
     * 4-  white rook
     * 5-  white pawn
     * 6-  black king
     * 7-  black queen
     * 8-  black bishop
     * 9-  black knight
     * 10- black rook
     * 11- black pawn
     * 12- white to move (0)/black to move (1)
     * 13- castling: white-kingside (0)/white-queenside (1)/black-kingside (0)/black-queenside (1)
     * 14- en passant file (if any)
     */
    static uint64_t TRANSPOSITION_HASHES[15][64] = {0};
    static std::unordered_map<uint64_t, TTEntry> transpositionTable;
    static std::unordered_map<uint64_t, float> qTranspositionTable; // for quiescence

    void initTT() {
        std::mt19937_64 engine(67);

        for (size_t i = 0; i < 15; i++) {
            for (size_t j = 0; j < 64; j++) {
                TRANSPOSITION_HASHES[i][j] = engine();
            }
        }

        transpositionTable.reserve(32000000); // 32 MILLION POSITIONS!!!!!
        transpositionTable.reserve(1000000); // 1 MILLION POSITIONS!!!!!
    }

    uint64_t getHash(const Board& board) {
        uint64_t hash = 0;

        for (int i = 0; i < 12; i++) {
            uint64_t bitboard = board.bitboards[0][i];
            while (bitboard) {
                uint8_t index = countTrailingZeros(bitboard);
                bitboard &= ~getMask(index);
                hash ^= TRANSPOSITION_HASHES[i][index];
            }
        }

        hash ^= TRANSPOSITION_HASHES[12][SIDE_WHITE];
        if (board.state.canCastle(SIDE_WHITE, KING)) hash ^= TRANSPOSITION_HASHES[13][0];
        if (board.state.canCastle(SIDE_WHITE, QUEEN)) hash ^= TRANSPOSITION_HASHES[13][1];
        if (board.state.canCastle(SIDE_BLACK, KING)) hash ^= TRANSPOSITION_HASHES[13][2];
        if (board.state.canCastle(SIDE_BLACK, QUEEN)) hash ^= TRANSPOSITION_HASHES[13][3];

        if (IS_VALID_SQUARE(board.state.enpassantSquare)) {
            hash ^= TRANSPOSITION_HASHES[14][getFile(board.state.enpassantSquare)];
        }

        return hash;
    }

    static const float STATIC_PIECE_VALUES[6] = {
        1000, 9, 3.2, 3, 5, 1
    };

    static const float PIECE_SQUARE_TABLES[6][64] = {
        { // KING
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -20,-30,-30,-40,-40,-30,-30,-20,
            -10,-20,-20,-20,-20,-20,-20,-10,
            20, 20,  0,  0,  0,  0, 20, 20,
            20, 30, 10,  0,  0, 10, 30, 20
        },
        { // QUEEN
            -20,-10,-10, -5, -5,-10,-10,-20,
            -10,  0,  0,  0,  0,  0,  0,-10,
            -10,  0,  5,  5,  5,  5,  0,-10,
            -5,  0,  5,  5,  5,  5,  0, -5,
            0,  0,  5,  5,  5,  5,  0, -5,
            -10,  5,  5,  5,  5,  5,  0,-10,
            -10,  0,  5,  0,  0,  0,  0,-10,
            -20,-10,-10, -5, -5,-10,-10,-20
        },
        { // BISHOP
            -20,-10,-10,-10,-10,-10,-10,-20,
            -10,  0,  0,  0,  0,  0,  0,-10,
            -10,  0,  5, 10, 10,  5,  0,-10,
            -10,  5,  5, 10, 10,  5,  5,-10,
            -10,  0, 10, 10, 10, 10,  0,-10,
            -10, 10, 10, 10, 10, 10, 10,-10,
            -10,  5,  0,  0,  0,  0,  5,-10,
            -20,-10,-10,-10,-10,-10,-10,-20,
        },
        { // KNIGHT
            -50,-40,-30,-30,-30,-30,-40,-50,
            -40,-20,  0,  0,  0,  0,-20,-40,
            -30,  0, 10, 15, 15, 10,  0,-30,
            -30,  5, 15, 20, 20, 15,  5,-30,
            -30,  0, 15, 20, 20, 15,  0,-30,
            -30,  5, 10, 15, 15, 10,  5,-30,
            -40,-20,  0,  5,  5,  0,-20,-40,
            -50,-40,-30,-30,-30,-30,-40,-50,
        },
        { // ROOK
            0,  0,  0,  0,  0,  0,  0,  0,
            5, 10, 10, 10, 10, 10, 10,  5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            0,  0,  0,  5,  5,  0,  0,  0
        },
        { // PAWN
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
        uint8_t oppositeColor = OPPOSITE_SIDE(activeColor);
        // material value
        for (size_t i = 1; i < sizeof(STATIC_PIECE_VALUES) / sizeof(float); i++) {
            eval += STATIC_PIECE_VALUES[i] * board.countPieces(activeColor, i);
            eval -= STATIC_PIECE_VALUES[i] * board.countPieces(oppositeColor, i);
        }

        float mobility = 0;
        float pieceSquareCompat = 0;
        for (int i = 1; i < 6; i++) {
            iterateIndices(board.bitboards[activeColor][i], [i, &mobility, &board, &pieceSquareCompat, activeColor](uint8_t index) -> void {
                mobility += countOnes(board.plMoveBB(i, index, activeColor));
                size_t pstIndex = (board.state.activeColor == SIDE_WHITE) ? 63 - i : i;
                pieceSquareCompat += PIECE_SQUARE_TABLES[i][pstIndex];
            });
            iterateIndices(board.bitboards[oppositeColor][i], [i, &mobility, &board, &pieceSquareCompat, oppositeColor](uint8_t index) -> void {
                mobility -= countOnes(board.plMoveBB(i, index, oppositeColor));
                size_t pstIndex = (board.state.activeColor == SIDE_WHITE) ? i : 63 - i;
                pieceSquareCompat -= PIECE_SQUARE_TABLES[i][pstIndex];
            });
        }
        
        return eval + mobility * .1 + pieceSquareCompat * .001;
    }

    int pruned;

    float quiesce(Board& board, float alpha, float beta) {
        // stand pat
        float bestValue = staticEvaluate(board);
        if (bestValue >= beta) return bestValue;
        alpha = std::max(alpha, bestValue);

        uint64_t boardHash = getHash(board);
        if (transpositionTable.count(boardHash)) {
            return transpositionTable[boardHash].eval;
        }
        if (qTranspositionTable.count(boardHash)) {
            return qTranspositionTable[boardHash];
        }

        std::vector<Move> moves = board.generatePLMoves();
        uint64_t opponentPieces = getOccupiedBitboard(board.bitboards[OPPOSITE_SIDE(board.state.activeColor)]);
        for (const Move& move : moves)  {
            uint64_t toMask = getMask(move.to);
            if (!(board.plMoveBB(move.pieceType, move.to, board.state.activeColor)
                    & board.bitboards[OPPOSITE_SIDE(board.state.activeColor)][KING]) // check
                && !(toMask & opponentPieces) // capture
                && !IS_VALID_PIECE(move.promotionType)) continue; // promotion
            UnmakeMove unmake = board.makeMove(move);
            if (unmake.isValid()) {
#ifdef BOT_PERF_CTR
                consideredMoves++;
#endif
                float score = -quiesce(board, -beta, -alpha);
                board.unmakeMove(unmake);
                if (score > bestValue) {
                    bestValue = score;
                    alpha = std::max(alpha, score);
                }
                if (score >= beta) {
                    qTranspositionTable[boardHash] = score;
                    pruned++;
                    return bestValue;
                } // failsoft
            }
        }

        qTranspositionTable[boardHash] = bestValue;

        return bestValue;
    }

    float evaluate(Board& board, float alpha, float beta, int depth) {
        if (depth == 0) return quiesce(board, alpha, beta);

        uint64_t boardHash = getHash(board);

        if (transpositionTable.count(boardHash) && transpositionTable[boardHash].depth >= depth) {
            return transpositionTable[boardHash].eval;
        }

        float bestValue = -MATE_EVAL;
        std::vector<Move> moves = board.generatePLMoves();

        // PV
        auto maxIt = std::max_element(moves.begin(), moves.end(),
            [&board](const Move& a, const Move& b) {
                UnmakeMove unmake = board.makeMove(a);
                if (!unmake.isValid()) return false;
                uint64_t hashA = getHash(board);
                if (!transpositionTable.count(hashA)) {
                    transpositionTable[hashA] = { -1, staticEvaluate(board) };
                }
                board.unmakeMove(unmake);

                unmake = board.makeMove(b);
                if (!unmake.isValid()) return true;
                uint64_t hashB = getHash(board);
                if (!transpositionTable.count(hashB)) {
                    transpositionTable[hashB] = { -1, staticEvaluate(board) };
                }
                board.unmakeMove(unmake);

                return transpositionTable[hashA].eval > transpositionTable[hashB].eval;
            }
        );
        if (maxIt != moves.begin()) {
            std::iter_swap(moves.begin(), maxIt);
        }

        for (const Move& move : moves)  {
            UnmakeMove unmake = board.makeMove(move);
            if (unmake.isValid()) {
#ifdef BOT_PERF_CTR
                consideredMoves++;
#endif
                float score = -evaluate(board, -beta, -alpha, depth - 1);
                board.unmakeMove(unmake);
                if (score > bestValue) {
                    bestValue = score;
                    alpha = std::max(alpha, score);
                }
                if (score >= beta) {
                    transpositionTable[boardHash] = { depth, bestValue };
                    pruned++;
                    return bestValue;
                } // failsoft
            }
        }

        transpositionTable[boardHash] = { depth, bestValue };
        return bestValue;
    }

    EvalNode::EvalNode(Board& board, float eval) : board(board), eval(eval) {}

    Engine::Engine(Board& board) {
        head = new EvalNode(board, 0);
    }

    Move Engine::getBestMove(uint16_t depth) {
        Move bestMove(0, 0, 0); // placeholder values
        float bestEval = -std::numeric_limits<float>::infinity();
        
        std::vector<Move> plMoves = head->board.generatePLMoves();
        
        for (const Move& move : plMoves) {
            UnmakeMove unmakeMove = head->board.makeMove(move);
            if (unmakeMove.isValid()) {
                std::cout << "Attempting " << indexToPrettyString(move.from)
                          << " to " << indexToPrettyString(move.to) << ": ";
                int64_t ms = getCurrentMs();
                float eval;
                int prepruned = pruned;
                for (int i = 1; i <= depth; i++) { // fake iterative deepening optimization
                    std::cout << std::flush << "..." << std::to_string(i);
                    eval = -evaluate(head->board, // idk y its negative (it breaks otherwise) but so be it
                        -std::numeric_limits<float>::infinity(),
                        std::numeric_limits<float>::infinity(), i);
                }

                head->board.unmakeMove(unmakeMove);

                std::cout << ": " << std::to_string(eval)
                          << " (" << std::to_string(getCurrentMs() - ms) << "ms, "
                          << std::to_string(pruned - prepruned) + " pruned)" << std::endl;

                if (eval > bestEval) {
                    bestEval = eval;
                    bestMove = move;
                }
            }
        }
        std::cout << std::endl;
#ifdef BOT_PERF_CTR
        std::cout << "Evaluation (" << std::to_string(consideredMoves) << " nodes searched): " << std::to_string(bestEval) << std::endl;
#else
        std::cout << "Evaluation: " << std::to_string(bestEval) << std::endl;
#endif
        return bestMove;
    }
}
