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

    void GameState::toggleCastling(int color, int sidePiece) {
        uint8_t mask = 1 << ((sidePiece - KING) + color * 2);
        castling ^= mask;
    }

    bool GameState::canCastle(int color, int sidePiece) {
        uint8_t mask = 1 << ((sidePiece - KING) + color * 2);
        return (castling & mask) != 0;
    }

    Board::Board() {
        memset(bitboards, 0, sizeof(bitboards));
        state = { SIDE_WHITE, 15, 0, 0, 0 };
    }

    Board::Board(const std::string& fen) : Board() {
        std::vector<std::string> fenParts = split(fen, " ");
        
        // piece positions
        std::vector<std::string> positions = split(fenParts[0], "/");
        for (int rank = 0; rank < 8; rank++) {
            const std::string& row = positions[7 - rank];
            int file = 0;
            for (int strIndex = 0; strIndex < 8; strIndex++) {
                switch(row[strIndex]) {
                    case 'K':
                        bitboards[SIDE_WHITE][KING] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'Q':
                        bitboards[SIDE_WHITE][QUEEN] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'B':
                        bitboards[SIDE_WHITE][BISHOP] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'N':
                        bitboards[SIDE_WHITE][KNIGHT] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'R':
                        bitboards[SIDE_WHITE][ROOK] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'P':
                        bitboards[SIDE_WHITE][PAWN] |= (1ULL << getIndex(rank, file));
                    break;

                    case 'k':
                        bitboards[SIDE_BLACK][KING] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'q':
                        bitboards[SIDE_BLACK][QUEEN] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'b':
                        bitboards[SIDE_BLACK][BISHOP] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'n':
                        bitboards[SIDE_BLACK][KNIGHT] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'r':
                        bitboards[SIDE_BLACK][ROOK] |= (1ULL << getIndex(rank, file));
                    break;
                    case 'p':
                        bitboards[SIDE_BLACK][PAWN] |= (1ULL << getIndex(rank, file));
                    break;

                    default: // numeric
                        int val = row[file] - '0';
                        file += val - 1;
                    break;
                }
                file++;

                if (file < 0) break;
            }
        }
    
        // turn
        state.activeColor = (fenParts[1] == "w") ? SIDE_WHITE : SIDE_BLACK;

        // castling
        const std::string& castlingString = fenParts[2];
        if (castlingString.find('k')) state.toggleCastling(SIDE_WHITE, KING);
        if (castlingString.find('q')) state.toggleCastling(SIDE_WHITE, QUEEN);
        if (castlingString.find('K')) state.toggleCastling(SIDE_BLACK, KING);
        if (castlingString.find('Q')) state.toggleCastling(SIDE_BLACK, QUEEN);

        // en passant
        const std::string& passantString = fenParts[3];
        if (passantString.length() == 2) {
            uint8_t passantFile = passantString[0] - 'A';
            uint8_t passantRank = passantString[1] - '0' - 1;
            state.enpassantSquare = getIndex(passantRank, passantFile);
        }

        // half moves
        state.halfMoveClock = std::stoi(fenParts[4]);
        
        // full moves
        state.moveCount = std::stoi(fenParts[5]);
    }

    uint64_t getMask(int rank, int file) {
        return 1ULL << getIndex(rank, file);
    }

    uint8_t getIndex(int rank, int file) {
        return (rank * 8) + file;
    }
} // namespace choco
