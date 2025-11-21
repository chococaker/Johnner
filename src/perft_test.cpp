#include <string>
#include <cstdlib>

#include "board.h"
#include "perft.h"

int main(int argc, char* argv[]) {
    int depth = std::atoi(argv[1]);
    std::string fen = std::string(argv[2]);

    choco::Board board(fen);
    choco::perft(board, depth, true, true);
}
