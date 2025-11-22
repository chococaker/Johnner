#include "board.h"
#include "search.h"
#include "macros.h"

#include <iostream>
#include <random>
#include <limits>
#include <stdexcept>

void runEvaluator() {
    choco::initBitboards();
    choco::initTT();
    std::cout << "Finished init" << std::endl;

    choco::Board bb = choco::Board("3r2k1/p4pp1/1p2p2p/1Bp1N3/P3RP2/6P1/r3P1KP/8 b - - 1 28");

    std::cout << (bb.state.activeColor == SIDE_WHITE ? "White" : "Black") << " to move: " << std::endl;
    choco::Search engine(bb);
    choco::Move move = engine.getBestMove(3);
    bb.makeMove(move);

    std::cout << choco::pieceToPrettyString(move.pieceType)
              << " "
              << choco::indexToPrettyString(move.from)
              << " to " << choco::indexToPrettyString(move.to);
    
    if (IS_VALID_PIECE(move.promotionType)) {
        std::cout << " (" << choco::pieceToPrettyString(move.promotionType) << ")";
    }
    std::cout << std::endl;
    
    std::cout << choco::boardToPrettyString(bb) << "\n" << std::endl;

    std::cout << "Done" << std::endl;
}

int main() {
    runEvaluator();

    return 0;
}
