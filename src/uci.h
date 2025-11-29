#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

#include "search.h"
#include "bithelpers.h"

namespace choco {
    class UciOptions {
    public:
        struct Option {
            std::string name;
            std::string type;
            std::string value;
            std::string defaultVal;
            std::string min;
            std::string max;
        };

        template<typename T>
        T get(const std::string &name);

        std::unordered_map<std::string, choco::UciOptions::Option>::iterator begin() {
            return options.begin();
        }
        std::unordered_map<std::string, choco::UciOptions::Option>::iterator end() {
            return options.end();
        }
    private:
        std::unordered_map<std::string,  Option> options;
    };

    class UciInstance {
    public:
        UciInstance();

        void loop();
        void processLine(const std::string& line);
    private:
        void applyOptions();

        // commands
        void uci();
        void position(const std::string& line);
        void uciNewGame();
        void go(const std::string& line);
        void quit();
        void stop();
        void isReady();
        void setOption(const std::string& in);

        UciOptions options;
        Search search;
    };


    template<typename T>
    T UciOptions::get(const std::string &name) {
        if constexpr (std::is_same_v<T, int>)
            return std::stoi(options[name].value);
        else if constexpr (std::is_same_v<T, float>)
            return std::stof(options[name].value);
        else if constexpr (std::is_same_v<T, uint64_t>)
            return std::stoull(options[name].value);
        else if constexpr (std::is_same_v<T, bool>)
            return options[name].value == "true";
        else
            return options[name].value;
    }

    inline Move uciToMove(const Board& board, const std::string& str) {
        std::string fromStr = str.substr(0, 2);
        std::string toStr = str.substr(2, 2);

        uint8_t from = (fromStr[0] - 'a') + (fromStr[1] - '1') * 8;
        uint8_t to = (toStr[0] - 'a') + (toStr[1] - '1') * 8;

        uint8_t pieceType = getPieceOnSquare(board.bitboards, from);
        
        Move move = { pieceType, from, to, INVALID_PIECE };

        if (str.size() == 4) return move;

        switch (str[4]) {
            case 'q': move.promotionType = QUEEN;  break;
            case 'n': move.promotionType = KNIGHT; break;
            case 'b': move.promotionType = BISHOP; break;
            case 'r': move.promotionType = ROOK;   break;
        };

        return move;
    }

    inline std::string moveToUci(const Move& move) {
        std::string fromStr = std::string(1, (move.from % 8) + 'a') + std::to_string(move.from / 8 + 1);
        std::string toStr = std::string(1, (move.to % 8) + 'a') + std::to_string(move.to / 8 + 1);

        std::string promotionStr = "";

        switch (move.promotionType) {
            case QUEEN:  promotionStr = "q"; break;
            case KNIGHT: promotionStr = "n"; break;
            case BISHOP: promotionStr = "b"; break;
            case ROOK:   promotionStr = "r"; break;
        }

        return fromStr + toStr + promotionStr;
    }
} // namespace choco
