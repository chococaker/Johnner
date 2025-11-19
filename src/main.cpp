#include "board.h"
#include "engine.h"

#include "macros.h"

#include <iostream>
#include <random>
#include <limits>

void runEvaluator() {
    choco::initBitboards();
    choco::initTT();

    choco::Board bb = choco::Board("3nk2r/1pp1B1pp/4P3/8/1p2N3/7P/PPP2P2/2K4R b k - 0 1");

    std::cout << (bb.state.activeColor == SIDE_WHITE ? "White" : "Black") << " to move: " << std::endl;
    choco::Engine engine(bb);
    choco::Move move = engine.getBestMove(5);
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
