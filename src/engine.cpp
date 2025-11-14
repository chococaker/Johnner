#include "engine.h"

#include <limits>
#include <algorithm>

#include "macros.h"

namespace choco {
    static const float MATE_EVAL = std::numeric_limits<float>::max();
    static const float MATE_EVAL_THRESHOLD = MATE_EVAL - 200;
    
    float staticEvaluate(const Board& board) {
        float eval = 0;
        // material value
        eval += 9 * board.countPieces(SIDE_WHITE, QUEEN);
        eval += 5 * board.countPieces(SIDE_WHITE, ROOK);
        eval += 3 * board.countPieces(SIDE_WHITE, KNIGHT);
        eval += 3 * board.countPieces(SIDE_WHITE, BISHOP);
        eval += 1 * board.countPieces(SIDE_WHITE, PAWN);

        eval -= 9 * board.countPieces(SIDE_BLACK, QUEEN);
        eval -= 5 * board.countPieces(SIDE_BLACK, ROOK);
        eval -= 3 * board.countPieces(SIDE_BLACK, KNIGHT);
        eval -= 3 * board.countPieces(SIDE_BLACK, BISHOP);
        eval -= 1 * board.countPieces(SIDE_BLACK, PAWN);
        
        return eval;
    }

    float alphabeta(Board& board, uint32_t depth, float alpha, float beta) {
        if (board.state.halfMoveClock == 50) return 0;
        if (!board.bitboards[board.state.activeColor][KING]) {
            return (board.state.activeColor == SIDE_WHITE) ? -MATE_EVAL : MATE_EVAL;
        }

        if (depth == 0) {
            return staticEvaluate(board);
        }
        std::vector<Move> moves = board.generatePLMoves();
        if (board.state.activeColor == SIDE_WHITE) {
            float maxEval = -std::numeric_limits<float>::infinity();
            for (const Move& move : moves) {
                Board newBoard = Board(board);
                if (newBoard.makeMove(move)) {
                    float eval = alphabeta(newBoard, depth - 1, alpha, beta);
                    maxEval = std::max(maxEval, eval);
                    if (maxEval >= beta) break; // beta cutoff
                    alpha = std::max(alpha, maxEval);
                }
            }

            if (maxEval < -MATE_EVAL_THRESHOLD) maxEval += 1;
            if (maxEval > MATE_EVAL_THRESHOLD) maxEval -= 1;
            return maxEval;
        } else {
            float minEval = std::numeric_limits<float>::infinity();
            for (const Move& move : moves) {
                Board newBoard = Board(board);
                if (newBoard.makeMove(move)) {
                    float eval = alphabeta(newBoard, depth - 1, alpha, beta);
                    minEval = std::min(minEval, eval);
                    if (minEval <= alpha) break; // alpha cutoff
                    beta = std::min(beta, minEval);
                }
            }
            if (minEval < -MATE_EVAL_THRESHOLD) minEval += 1;
            if (minEval > MATE_EVAL_THRESHOLD) minEval -= 1;
            return minEval;
        }
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
            Board newBoard = Board(head->board);
            if (newBoard.makeMove(move)) {
                std::cout << "Attempting " << indexToPrettyString(move.from)
                          << " to " << indexToPrettyString(move.to);
                float eval = alphabeta(newBoard, depth,
                    -std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity());

                std::cout << "  - " << indexToPrettyString(move.from)
                          << " to " << indexToPrettyString(move.to)
                          << ": " << std::to_string(eval) << std::endl;

                if (head->board.state.activeColor == SIDE_WHITE && eval > bestEval) {
                    bestEval = eval;
                    bestMove = move;
                } else if (head->board.state.activeColor == SIDE_BLACK && eval < bestEval) {
                    bestEval = eval;
                    bestMove = move;
                }
            }
        }
        std::cout << "Evaluation: " << std::to_string(bestEval) << std::endl;
        return bestMove;
    }
    void Engine::playMove(const Move& move) const {
        
    }
}