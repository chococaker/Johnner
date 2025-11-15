#include "engine.h"

#include <limits>
#include <algorithm>

#include "macros.h"

namespace choco {
    static const float MATE_EVAL = 32000;
    static const float MATE_EVAL_THRESHOLD = MATE_EVAL - 500; // I can spot mate in 500, technically!
    
    float staticEvaluate(const Board& board) {
        float eval = 0;
        // material value
        eval += 9 * board.countPieces(board.state.activeColor, QUEEN);
        eval += 5 * board.countPieces(board.state.activeColor, ROOK);
        eval += 3 * board.countPieces(board.state.activeColor, KNIGHT);
        eval += 3 * board.countPieces(board.state.activeColor, BISHOP);
        eval += 1 * board.countPieces(board.state.activeColor, PAWN);

        uint8_t oppositeSide = OPPOSITE_SIDE(board.state.activeColor);
        eval -= 9 * board.countPieces(oppositeSide, QUEEN);
        eval -= 5 * board.countPieces(oppositeSide, ROOK);
        eval -= 3 * board.countPieces(oppositeSide, KNIGHT);
        eval -= 3 * board.countPieces(oppositeSide, BISHOP);
        eval -= 1 * board.countPieces(oppositeSide, PAWN);
        
        return eval;
    }

    float quiesce(Board& board, float alpha, float beta) {
        // stand pat
        int bestValue = staticEvaluate(board);
        if(bestValue >= beta) return bestValue;
        if(bestValue > alpha) alpha = bestValue;

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
                float score = -quiesce(board, -beta, -alpha);
                board.unmakeMove(unmake);
                if(score > bestValue) {
                    bestValue = score;
                    if (score > alpha) alpha = score;
                }
                if (score >= beta) return bestValue; // failsoft
            }
        }

        return bestValue;
    }

    float evaluate(Board& board, float alpha, float beta, int depth) {
        if (depth == 0) return quiesce(board, alpha, beta);

        float bestValue = -MATE_EVAL;
        std::vector<Move> moves = board.generatePLMoves();
        for (const Move& move : moves)  {
            UnmakeMove unmake = board.makeMove(move);
            if (unmake.isValid()) {
                float score = -evaluate(board, -beta, -alpha, depth - 1);
                board.unmakeMove(unmake);
                if (score > bestValue) {
                    bestValue = score;
                    if (score > alpha) alpha = score;
                }
                if (score >= beta)
                    return bestValue; // failsoft
            }
        }

        return bestValue;
    }

    EvalNode::EvalNode(Board& board, float eval) : board(board), eval(eval) {}

    Engine::Engine(Board& board) {
        head = new EvalNode(board, 0);
    }

    Move Engine::getBestMove(uint16_t depth) {
        Move bestMove(0, 0, 0); // placeholder values
        float bestEval = (head->board.state.activeColor == SIDE_WHITE)
                ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity();
        
        for (const Move& move : head->board.generatePLMoves()) {
            UnmakeMove unmakeMove = head->board.makeMove(move);
            if (unmakeMove.isValid()) {
                std::cout << "Attempting " << indexToPrettyString(move.from)
                          << " to " << indexToPrettyString(move.to) << ": ";
                float eval = evaluate(head->board,
                    -std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity(), depth);
                    head->board.unmakeMove(unmakeMove);

                std::cout << std::to_string(eval) << std::endl;

                if (head->board.state.activeColor == SIDE_WHITE && eval > bestEval) {
                    bestEval = eval;
                    bestMove = move;
                } else if (head->board.state.activeColor == SIDE_BLACK && eval < bestEval) {
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
