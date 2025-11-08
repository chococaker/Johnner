#include "bitboard.h"
#include <iostream>
#include <cstring>
#include <vector>
#include "macros.h"

namespace choco {
    namespace {
        // https://stackoverflow.com/a/46931770
        std::vector<std::string> split(std::string s, std::string delimiter) {
            size_t pos_start = 0, pos_end, delim_len = delimiter.length();
            std::string token;
            std::vector<std::string> res;
            while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
                token = s.substr (pos_start, pos_end - pos_start);
                pos_start = pos_end + delim_len;
                res.push_back(token);
            }
            res.push_back(s.substr (pos_start));
            return res;
        }
    }

    Board::Board() {
        memset(bitboards, 0, sizeof(bitboards));
    }
    Board::Board(const std::string& fen) : Board() {
        std::vector<std::string> fenParts = split(fen, " ");
        
        // piece positions
        std::vector<std::string> positions = split(fenParts[0], "/");
        for (int rank = 0; rank < 8; rank++) {
            const std::string& row = positions[7 - rank];
            for (int file = 0; file < 8; file++) {
                switch(row[file]) {
                    case 'k':
                        bitboards[SIDE_WHITE][KING] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'q':
                        bitboards[SIDE_WHITE][QUEEN] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'b':
                        bitboards[SIDE_WHITE][BISHOP] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'n':
                        bitboards[SIDE_WHITE][KNIGHT] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'r':
                        bitboards[SIDE_WHITE][ROOK] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'p':
                        bitboards[SIDE_WHITE][PAWN] |= (1ULL << getIndex(rank, file));
                    break;

                    case 'K':
                        bitboards[SIDE_BLACK][KING] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'Q':
                        bitboards[SIDE_BLACK][QUEEN] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'B':
                        bitboards[SIDE_BLACK][BISHOP] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'N':
                        bitboards[SIDE_BLACK][KNIGHT] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'R':
                        bitboards[SIDE_BLACK][ROOK] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'P':
                        bitboards[SIDE_BLACK][PAWN] |= (1ULL << getIndex(rank, file));
                    break;

                    default: // numeric
                        size_t val = row[file] - '0';
                        file += val;
                    break;
                }
            }
        }
    }

    size_t Board::getPieceType(int rank, int file) const {
        return BISHOP;
    }

    size_t Board::getPieceColor(int rank, int file) const {
        return SIDE_WHITE;
    }

    uint64_t getMask(int rank, int file) {
        static const uint64_t LAST_BIT = 63;
        return 1ULL << (LAST_BIT - (rank * 8) - file);
    }

    int getIndex(int rank, int file) {
        return (rank * 8) + file;
    }
} // namespace choco

std::ostream& operator<<(std::ostream& os, const choco::Board& obj) {
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 7; file >= 0; file--) {
            uint64_t mask = choco::getMask(rank, file);
            os << (((bool) (obj.bitboards[SIDE_WHITE][KING] & mask)));
            os << " ";
        }
        os << std::endl;
    }
    return os;
}
