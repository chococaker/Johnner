#include <iostream>

#include "uci.h"
#include "board.h"
#include "search.h"

int main() {
    choco::initBitboards();
    choco::initTT();

    choco::UciInstance inst{};

    std::string input;
    std::cin >> std::ws;

    while (true) {
        std::getline(std::cin, input);
        inst.processLine(input);
    }
}
