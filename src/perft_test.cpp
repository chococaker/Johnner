#include <string>
#include <cstdlib>
 #include <chrono>

#include "board.h"
#include "perft.h"

#define DEBUG_PERFT

int main(int argc, char* argv[]) {
    int depth = std::atoi(argv[1]);
    std::string fen = std::string(argv[2]);

#ifdef DEBUG_PERFT
    auto start = std::chrono::steady_clock::now();
#endif
    choco::Board board(fen);
    uint32_t nodesSearched = choco::perft(board, depth, true, true);
#ifdef DEBUG_PERFT
    auto end = std::chrono::steady_clock::now();
#endif

#ifdef DEBUG_PERFT
    std::chrono::duration<double> elapsed = end - start;
    double elapsed_seconds = elapsed.count();
    std::cout << "Elapsed time: " << elapsed_seconds << "s" << std::endl;
    std::cout << "Avg. nps: " << std::to_string(nodesSearched / elapsed_seconds) << std::endl;
    std::cout << "Avg. spn: " << std::to_string(elapsed_seconds / nodesSearched) << std::endl;
#endif
}
