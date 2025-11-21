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

    choco::Board bb = choco::Board("3r2k1/pp3pp1/5P2/2p3P1/4p3/2n5/4r2P/5K1R b - - 0 1");

    std::cout << (bb.state.activeColor == SIDE_WHITE ? "White" : "Black") << " to move: " << std::endl;
    choco::Search engine(bb);
    choco::Move move = engine.getBestMove(7);
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
