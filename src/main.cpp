#include <iostream>

#include "uci.h"
#include "board.h"
#include "search.h"
#include "zobrist.h"

int main() {
    choco::initBitboards();
    choco::initZobrist();

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
