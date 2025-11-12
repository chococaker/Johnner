#include "engine.h"

#include "macros.h"

namespace choco {
    float evaluatePosition(const Board& board) {
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

    Move Engine::getBestMove(uint16_t depth) const {
        
    }
    void Engine::playMove(const Move& move) const {
        
    }
}