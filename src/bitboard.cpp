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

        
        uint8_t countTrailingZeros(uint64_t n) {
            if (n == 0) {
                return 64;
            }
            #if defined(__GNUC__) || defined(__clang__)
                return __builtin_ctzll(n);
            #elif defined(_MSC_VER)
                return __tzcnt_u64(n);
            #else // fallback
                unsigned int count = 0;
                while ((n & 1) == 0 && count < 64) {
                    n >>= 1;
                    count++;
                }
                return count;
            #endif
        }

        std::vector<Move> extractMoves(uint64_t bitboard, uint8_t offset) {
            std::vector<Move> moves;
            while (bitboard != 0) {
                uint8_t index = countTrailingZeros(bitboard);
                bitboard &= ~(1ULL << index);
                Move move = { index, index };
                moves.push_back(move);
            }

            return moves;
        }

        uint64_t getOccupiedBitboard(const uint64_t bitboards[2][6]) {
            return getOccupiedBitboard(bitboards[0]) | getOccupiedBitboard(bitboards[1]);
        }

        uint64_t getEmptyBitboard(const uint64_t bitboards[2][6]) {
            return ~getOccupiedBitboard(bitboards);
        }

        uint64_t getOccupiedBitboard(const uint64_t bitboards[6]) {
            uint64_t bitboard = 0;
            for (size_t i = 0; i < sizeof(bitboards); i++) {
                bitboard |= bitboards[i];
            }
            return bitboard;
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
                        putPiece(SIDE_WHITE, KING, rank, file);
                    break;
                    case 'Q':
                        putPiece(SIDE_WHITE, QUEEN, rank, file);
                    break;
                    case 'B':
                        putPiece(SIDE_WHITE, BISHOP, rank, file);
                    break;
                    case 'N':
                        putPiece(SIDE_WHITE, KNIGHT, rank, file);
                    break;
                    case 'R':
                        putPiece(SIDE_WHITE, ROOK, rank, file);
                    break;
                    case 'P':
                        putPiece(SIDE_WHITE, PAWN, rank, file);
                    break;

                    case 'k':
                        putPiece(SIDE_BLACK, KING, rank, file);
                    break;
                    case 'q':
                        putPiece(SIDE_BLACK, QUEEN, rank, file);
                    break;
                    case 'b':
                        putPiece(SIDE_BLACK, BISHOP, rank, file);
                    break;
                    case 'n':
                        putPiece(SIDE_BLACK, KNIGHT, rank, file);
                    break;
                    case 'r':
                        putPiece(SIDE_BLACK, ROOK, rank, file);
                    break;
                    case 'p':
                        putPiece(SIDE_BLACK, PAWN, rank, file);
                    break;

                    default: // numeric
                        int val = row[file] - '0';
                        file += val - 1;
                    break;
                }
                file++;
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

    void Board::putPiece(uint8_t side, uint8_t piece, uint8_t rank, uint8_t file) {
        bitboards[side][piece] |= (1ULL << getIndex(rank, file));
    }
    void Board::removePiece(uint8_t side, uint8_t piece, uint8_t rank, uint8_t file) {
        bitboards[side][piece] ^= (1ULL << getIndex(rank, file));
    }
    void Board::movePiece(uint8_t side, uint8_t piece, uint8_t rankFrom, uint8_t fileFrom, uint8_t rankTo, uint8_t fileTo) {
        removePiece(side, piece, rankFrom, fileFrom);
        putPiece(side, piece, rankTo, fileTo);
    }

    std::vector<Move> Board::generateWhitePawnMoves() const {
        // pushes
        uint64_t emptySquares = getEmptyBitboard(bitboards);
        uint64_t pushedPawns = ((bitboards[SIDE_WHITE][PAWN] & ~BITBOARD_RANK_7) & emptySquares) << 8;
        uint64_t doublePushedPawns = ((pushedPawns & BITBOARD_RANK_3) & emptySquares) << 8;
        // ...
    }

    uint64_t getMask(int rank, int file) {
        return 1ULL << getIndex(rank, file);
    }

    uint8_t getIndex(int rank, int file) {
        return (rank * 8) + file;
    }
} // namespace choco
