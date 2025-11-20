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

    choco::Board bb = choco::Board("r1bq1rk1/ppp2ppp/2n2n2/4p3/1b2P3/2NP1NP1/PP1B1PBP/R2QK2R b KQhq - 0 1");

    std::cout << (bb.state.activeColor == SIDE_WHITE ? "White" : "Black") << " to move: " << std::endl;
    choco::Search engine(bb);
    choco::Move move = engine.getBestMove(6);
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

void runMoveGenTest() {
    choco::initBitboards();

    choco::Board bb = choco::Board("rnb1k1nr/p1pp1ppp/1p6/2P5/2P1p3/5N2/PB1PPPPP/1N2KB1R w KAkq - 0 1");

    std::cout << choco::boardToPrettyString(bb) << "\n" << std::endl;
}

int main() {
    runEvaluator();

    return 0;
}
