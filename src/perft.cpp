#include <iostream>
#include <cstdlib>
#include <string>

#include "board.h"
#include "bithelpers.h"

namespace choco {
    unsigned char pieceCharToType(uint8_t pieceType) {
        switch (pieceType) {
            case 'K':
                return KING;
            case 'Q':
                return QUEEN;
                break;
            case 'N':
                return KNIGHT;
                break;
            case 'B':
                return BISHOP;
            case 'R':
                return ROOK;
            case 'P':
                return PAWN;
        }
        return 'X';
    }

    unsigned char pieceTypeToChar(uint8_t pieceType) {
        switch (pieceType) {
            case KING:
                return 'K';
            case QUEEN:
                return 'Q';
            case KNIGHT:
                return 'N';
            case BISHOP:
                return 'B';
            case ROOK:
                return 'R';
            case PAWN:
                return 'P';
        }
        return 'X';
    }

    choco::Move toMove(const std::string& uciMove) {
        choco::Move move;

        uint8_t from = choco::getIndex(uciMove[0] - 'a', uciMove[1] - '1');
        uint8_t to = choco::getIndex(uciMove[2] - 'a', uciMove[3] - '1');

        move.from = from;
        move.to = to;

        // promotion
        if (uciMove.length() == 5) {
            move.pieceType = pieceCharToType(uciMove[4]);
        }

        return move;
    }

    uint32_t propagate(int depth, choco::Board& board) {
        if (depth == 0) return 1;

        uint32_t nodesSearched = 0;

        for (const choco::Move& move : board.generatePLMoves()) {
            choco::UnmakeMove unmakeMove = board.makeMove(move);
            if (!unmakeMove.isValid()) continue;

            nodesSearched += propagate(depth - 1, board);
            board.unmakeMove(unmakeMove);
        }
        
        return nodesSearched;
    }

    uint32_t perft(Board& board, int depth, bool initBB, bool print) {
        if (depth == 0) {
            std::cout << "1" << std::endl;
            return 1;
        }

        uint32_t nodesSearched = 0;

        if (initBB) {
            choco::initBitboards();
        }

        for (const choco::Move& move : board.generatePLMoves()) {
            choco::UnmakeMove unmakeMove = board.makeMove(move);
            if (!unmakeMove.isValid()) continue;
            
            if (IS_VALID_PIECE(move.promotionType)) {
                std::cout << pieceCharToType(move.promotionType);
            }

            uint32_t nodesThisTime = propagate(depth - 1, board);
            nodesSearched += nodesThisTime;
            board.unmakeMove(unmakeMove);

            if (print) {
                std::cout << (char)(choco::getFile(move.from) + 'a') << (char)(choco::getRank(move.from) + '1')
                        << (char)(choco::getFile(move.to) + 'a') << (char)(choco::getRank(move.to) + '1')
                        << " "
                        << std::to_string(nodesThisTime) << std::endl;
            }
        }

        if (print) {
            std::cout << "\n" << std::to_string(nodesSearched) << std::endl;
        }

        return nodesSearched;
    }
} // namespace choco
