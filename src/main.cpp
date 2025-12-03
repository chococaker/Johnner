#include <iostream>

#include "uci.h"
#include "board.h"
#include "search.h"
#include "zobrist.h"
#include "movegen.h"

int main() {
    choco::initZobrist();
    choco::initMoveGen();

    choco::UciInstance inst{};

    std::string input;
    std::cin >> std::ws;

    while (true) {
        if (!std::getline(std::cin, input) && std::cin.eof()) {
            input = "quit";
        }

        inst.processLine(input);

        if (input == "quit") break;
    }
}
