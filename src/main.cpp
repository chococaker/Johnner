#include "board.h"
#include "search.h"
#include "macros.h"

#include <iostream>
#include <random>
#include <limits>
#include <stdexcept>
#include <filesystem>

void runEvaluator() {
    choco::initBitboards();
    choco::initTT();
    std::cout << "Finished init" << std::endl;

    choco::Board bb = choco::Board("8/1Q2qk2/6n1/B7/5p1p/8/5PPP/6K1 w - - 0 1");

    std::cout << (bb.state.activeColor == SIDE_WHITE ? "White" : "Black") << " to move: " << std::endl;
    choco::Search engine(bb);
    choco::Move move = engine.getBestMove(15);
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
