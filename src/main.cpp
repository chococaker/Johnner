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

    choco::Board bb = choco::Board("rnbqkbnr/1ppppppp/p7/8/8/2N5/PPPPPPPP/R1BQKBNR w KQkq - 0 2");

    std::cout << (bb.state.activeColor == SIDE_WHITE ? "White" : "Black") << " to move: " << std::endl;
    choco::Search engine(bb);
    engine.search({ .moveTime = 5000 });
    choco::Move move = engine.getBestMove();
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
