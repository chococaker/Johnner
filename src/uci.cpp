#include "uci.h"

#include <iostream>
#include <vector>
#include <string>

#include "str_util.h"
#include "macros.h"

namespace choco {
    UciInstance::UciInstance() : search(Board()) { }

    void UciInstance::processLine(const std::string& line) {
        std::vector<std::string> tokens = util::split(line, " ");

        if (tokens.empty()) {
            return;
        }

        if (tokens[0] == "uci") {
            uci();
        } else if (tokens[0] == "setoption") {
            // setOption(line);
        } else if (tokens[0] == "position") {
            position(line);
        } else if (tokens[0] == "go") {
            go(line);
        } else if (tokens[0] == "stop") {
            stop();
        } else if (tokens[0] == "ponderhit") {

        } else if (tokens[0] == "ucinewgame") {
            uciNewGame();
        } else if (tokens[0] == "isready") {
            isReady();
        } else if (tokens[0] == "quit") {
            quit();
        }
    }

    void UciInstance::go(const std::string& line) {
        std::vector<std::string> lineSplit = util::split(line, " ");
        SearchBounds searchBounds = {};
        searchBounds.moveTime = util::findElement<int64_t>(lineSplit, "movetime").value_or((int64_t)10000);
        search.search(searchBounds);
    }

    void UciInstance::quit() {
        search.stop();
    }

    void UciInstance::stop() {
        search.stop();
    }

    void applyOptions() {

    }

    void UciInstance::uci() {
        std::cout << "id name Johnner_v1" << std::endl;
        std::cout << "id author chococaker\n" << std::endl;
        for (const auto &[name, option]: options) {
            std::cout << "option name " << name << " type " << option.type << " default "
                        << option.defaultVal;
            if (!option.min.empty()) {
                std::cout << " min " << option.min << " max " << option.max;
            }
            std::cout << std::endl;
        }
        std::cout << "uciok" << std::endl;
    }

    void UciInstance::position(const std::string& line) {
        const int fenRange = util::findRange(line, "fen", "moves");

        const std::string fen = (line.contains("fen"))
                                ? line.substr(line.find("fen") + 4, fenRange)
                                : STARTING_POS;

        const std::string moves = line.contains("moves") ? line.substr(line.find("moves") + 6) : "";
        const std::vector<std::string> moveVec = util::split(moves, " ");

        search.setBoard<false>(fen);

        for (const std::string& move : moveVec) {
            search.playMove(uciToMove(search.getBoard(), move));
        }
    }

    void UciInstance::uciNewGame() {
        search.clearTT();
    }

    void UciInstance::isReady() {
        std::cout << "readyok" << std::endl;
    }
} // namespace choco
