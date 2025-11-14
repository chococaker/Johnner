#include "engine.h"

#include <limits>
#include <algorithm>

#include "macros.h"

namespace choco {
    static const float MATE_EVAL = 32000; // idk just cause
    static const float MATE_EVAL_THRESHOLD = MATE_EVAL - 200; // I can spot mate in 200!
    
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

    float evaluate(Board& board, int depth, float alpha, float beta) {
        if (board.state.halfMoveClock == 100) return 0;

        if (depth == 0) return staticEvaluate(board);
        std::vector<Move> moves = board.generatePLMoves();
        if (board.state.activeColor == SIDE_WHITE) {
            float maxEval = -MATE_EVAL_THRESHOLD;
            for (const Move& move : moves) {
                UnmakeMove unmakeMove = board.makeMove(move);
                if (unmakeMove.isValid()) {
                    float eval = evaluate(board, depth - 1, alpha, beta);
                    board.unmakeMove(unmakeMove);
                    maxEval = std::max(maxEval, eval);
                    if (maxEval >= beta) break; // beta cutoff
                    alpha = std::max(alpha, maxEval);
                }
            }

            if (maxEval < -MATE_EVAL_THRESHOLD) maxEval += 1;
            if (maxEval > MATE_EVAL_THRESHOLD) maxEval -= 1;
            return maxEval;
        } else {
            float minEval = MATE_EVAL_THRESHOLD;
            for (const Move& move : moves) {
                UnmakeMove unmakeMove = board.makeMove(move);
                if (unmakeMove.isValid()) {
                    float eval = evaluate(board, depth - 1, alpha, beta);
                    board.unmakeMove(unmakeMove);
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

    // float quiesce(Board& board, int depth, float alpha, float beta) {
    //     float eval = staticEvaluate(board);
    //
    //     // Stand Pat
    //     int bestVal = eval;
    //     if (bestVal >= beta) return bestVal;
    //     if (bestVal > alpha) alpha = bestVal;
    //
    //     // examine every capture
    //     for (int i = 0; i < 6; i++) {
    //         uint64_t pieces = board.bitboards[board.state.activeColor][i];
    //         uint64_t enemyPieces = getOccupiedBitboard(board.bitboards[OPPOSITE_SIDE(board.state.activeColor)]);
    //
    //         iterateIndices(pieces, [&board, enemyPieces](uint8_t index) -> void {
    //             uint64_t attacks = board.plRookMoveBB(index, board.state.activeColor) & enemyPieces;
    //             iterateIndices(attacks, [&board, enemyPieces](uint8_t index) -> void {
    //                 board.makeMove()
    //             });
    //         });
    //     }
    //
    //     return bestVal;
    // }

    EvalNode::EvalNode(Board& board, float eval) : board(board), eval(eval) {}

    Engine::Engine(Board& board) {
        head = new EvalNode(board, 0);
    }

    Move Engine::getBestMove(uint16_t depth) {
        Move bestMove(0, 0, 0); // placeholder values
        float bestEval = (head->board.state.activeColor == SIDE_WHITE)
                ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity();

        Board newBoard = Board(head->board);
        for (const Move& move : head->board.generatePLMoves()) {
            UnmakeMove unmakeMove = newBoard.makeMove(move);
            if (unmakeMove.isValid()) {
                std::cout << "Attempting " << indexToPrettyString(move.from)
                          << " to " << indexToPrettyString(move.to);
                float eval = evaluate(newBoard, depth,
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
                newBoard.unmakeMove(unmakeMove);
            }
        }
        std::cout << "Evaluation: " << std::to_string(bestEval) << std::endl;
        return bestMove;
    }
    void Engine::playMove(const Move& move) const {
        
    }
}