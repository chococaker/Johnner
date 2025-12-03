#include "bithelpers.h"

#include "board.h"

#include <cmath>
#include <sstream>
#include <cstring>
#include <vector>
#include <limits>
#include <random>
#include <functional>
#include <unordered_map>

#include "macros.h"
#include "types.h"
#include "str_util.h"
#include "bithelpers.h"
#include "movegen.h" // TEMPORARY legality check


#define ALL_OCCUPIED_SQUARES (occupiedSquares[SIDE_WHITE] | occupiedSquares[SIDE_BLACK])

namespace choco {
    const UnmakeMove INVALID_MOVE = {{INVALID_SQUARE, INVALID_SQUARE, INVALID_PIECE}, INVALID_PIECE};

    bool UnmakeMove::isValid() const {
        return IS_VALID_PIECE(move.pieceType);
    }

    inline void GameState::enableCastling(uint8_t color, uint8_t sidePiece) {
        uint8_t mask = 1 << ((sidePiece - KING) + color * 2);
        castling |= mask;
    }

    inline void GameState::disableCastling(uint8_t color, uint8_t sidePiece) {
        uint8_t mask = 1 << ((sidePiece - KING) + color * 2);
        castling &= ~mask;
    }

    bool GameState::canCastle(uint8_t color, uint8_t sidePiece) const {
        uint8_t mask = 1 << ((sidePiece - KING) + color * 2);
        return (castling & mask) != 0;
    }

    Board::Board() {
        memset(bitboards, 0, sizeof(bitboards));
        occupiedSquares[SIDE_WHITE] = 0;
        occupiedSquares[SIDE_BLACK] = 0;
        state = { SIDE_WHITE, 0, 0, INVALID_SQUARE, 0 };
    }

    Board::Board(std::string fen) : Board() {
        std::vector<std::string> fenParts = util::split(fen, " ");
        
        // piece positions
        std::vector<std::string> positions = util::split(fenParts[0], "/");
        for (int rank = 0; rank < 8; rank++) {
            const std::string& row = positions[7 - rank];
            int file = 0;
            
            for (size_t strIndex = 0; strIndex < row.length() && file < 8; strIndex++) {
                uint8_t index = getIndex(rank, file);
                switch(row[strIndex]) {
                    case 'K':
                        putPiece(SIDE_WHITE, KING, index);
                    break;
                    case 'Q':
                        putPiece(SIDE_WHITE, QUEEN, index);
                    break;
                    case 'B':
                        putPiece(SIDE_WHITE, BISHOP, index);
                    break;
                    case 'N':
                        putPiece(SIDE_WHITE, KNIGHT, index);
                    break;
                    case 'R':
                        putPiece(SIDE_WHITE, ROOK, index);
                    break;
                    case 'P':
                        putPiece(SIDE_WHITE, PAWN, index);
                    break;

                    case 'k':
                        putPiece(SIDE_BLACK, KING, index);
                    break;
                    case 'q':
                        putPiece(SIDE_BLACK, QUEEN, index);
                    break;
                    case 'b':
                        putPiece(SIDE_BLACK, BISHOP, index);
                    break;
                    case 'n':
                        putPiece(SIDE_BLACK, KNIGHT, index);
                    break;
                    case 'r':
                        putPiece(SIDE_BLACK, ROOK, index);
                    break;
                    case 'p':
                        putPiece(SIDE_BLACK, PAWN, index);
                    break;

                    default: // numeric
                        int val = row[strIndex] - '0';
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
        if (castlingString.find('K') != std::string::npos) state.enableCastling(SIDE_WHITE, KING);
        if (castlingString.find('Q') != std::string::npos) state.enableCastling(SIDE_WHITE, QUEEN);
        if (castlingString.find('k') != std::string::npos) state.enableCastling(SIDE_BLACK, KING);
        if (castlingString.find('q') != std::string::npos) state.enableCastling(SIDE_BLACK, QUEEN);

        // en passant
        const std::string& passantString = fenParts[3];
        if (passantString.length() == 2) {
            uint8_t passantFile = passantString[0] - 'a';
            uint8_t passantRank = passantString[1] - '0' - 1;
            state.enpassantSquare = getIndex(passantRank, passantFile);
        }

        // half moves
        state.halfMoveClock = (fenParts.size() >= 5 && !fenParts[4].empty()) ? std::stoi(fenParts[4]) : 0;
        
        // full moves
        state.moveCount = (fenParts.size() >= 6 && !fenParts[5].empty()) ? std::stoi(fenParts[5]) : 0;
    }

    Board::Board(const Board& other) {
        std::memcpy(bitboards, other.bitboards, sizeof(bitboards));
        occupiedSquares[SIDE_WHITE] = other.occupiedSquares[SIDE_WHITE];
        occupiedSquares[SIDE_BLACK] = other.occupiedSquares[SIDE_BLACK];
        state = other.state;
    }

    inline void Board::putPiece(uint8_t side, uint8_t piece, uint8_t index) {
        uint64_t mask = getMask(index);
        bitboards[side][piece] |= mask;
        occupiedSquares[side] |= mask;
    }
    inline void Board::removePiece(uint8_t side, uint8_t piece, uint8_t index) {
        uint64_t mask = ~getMask(index);
        bitboards[side][piece] &= ~getMask(index);
        occupiedSquares[side] &= mask;
    }
    UnmakeMove Board::makeMove(const Move& move) {
        UnmakeMove unmakeMove = { move, INVALID_PIECE, state };

        if (move == NULL_MOVE) {
            state.activeColor = oppositeSide(state.activeColor);
            return UnmakeMove(move, INVALID_PIECE, state);
        }

        uint64_t movementMask = getMask(move.from) | getMask(move.to);
        bitboards[state.activeColor][move.pieceType] ^= movementMask;
        occupiedSquares[state.activeColor] ^= movementMask;

        state.halfMoveClock++;

        // capture
        for (uint8_t i = 0; i < 6; i++) {
            uint64_t initialVal = bitboards[OPPOSITE_SIDE(state.activeColor)][i];
            removePiece(OPPOSITE_SIDE(state.activeColor), i, move.to);
            if (initialVal != bitboards[OPPOSITE_SIDE(state.activeColor)][i]) {
                state.halfMoveClock = 0;
                unmakeMove.pieceTaken = i;
                break;
            }
        }

        uint64_t illegalAttackSquares = 0; // squares that are not allowed be attacked if this move is played

        // en passant
        if (move.pieceType == PAWN && move.to == state.enpassantSquare) {
            int offset = (state.activeColor == SIDE_WHITE) ? 8 : -8;
            removePiece(OPPOSITE_SIDE(state.activeColor), PAWN, state.enpassantSquare - offset);
        }

        state.enpassantSquare = INVALID_SQUARE;

        if (move.pieceType == PAWN) {
            state.halfMoveClock = 0;
            // double push
            if (move.from - move.to == 16 || move.to - move.from == 16) {
                if (move.from - move.to == 16) {
                    state.enpassantSquare = move.from - 8;
                } else if (move.to - move.from == 16) {
                    state.enpassantSquare = move.from + 8;
                }
            }

            // promotion
            if (IS_VALID_PIECE(move.promotionType)) {
                removePiece(state.activeColor, PAWN, move.to);
                putPiece(state.activeColor, move.promotionType, move.to);
            }
        }

        // castling spaghetti
        if (move.pieceType == KING && (move.from == E1 || move.from == E8)) {
            if (move.to == C1 || move.to == G1) { // white
                illegalAttackSquares |= getMask(E1);
                if (move.to == C1) {
                    illegalAttackSquares |= getMask(D1);
                    removePiece(SIDE_WHITE, ROOK, A1);
                    putPiece(SIDE_WHITE, ROOK, D1);
                } else {
                    illegalAttackSquares |= getMask(G1);
                    removePiece(SIDE_WHITE, ROOK, H1);
                    putPiece(SIDE_WHITE, ROOK, F1);
                }
            } else if (move.to == C8 || move.to == G8) {
                illegalAttackSquares |= getMask(E8);
                if (move.to == C8) {
                    illegalAttackSquares |= getMask(D8);
                    removePiece(SIDE_BLACK, ROOK, A8);
                    putPiece(SIDE_BLACK, ROOK, D8);
                } else {
                    illegalAttackSquares |= getMask(F8);
                    removePiece(SIDE_BLACK, ROOK, H8);
                    putPiece(SIDE_BLACK, ROOK, F8);
                }
            }
            // king has moved; no more castling!!!!!!
            state.disableCastling(state.activeColor, KING);
            state.disableCastling(state.activeColor, QUEEN);
        }

        // rook has been taken OR moved
        if (move.from == A1 || move.to == A1) state.disableCastling(SIDE_WHITE, QUEEN);
        if (move.from == H1 || move.to == H1) state.disableCastling(SIDE_WHITE, KING);
        if (move.from == A8 || move.to == A8) state.disableCastling(SIDE_BLACK, QUEEN);
        if (move.from == H8 || move.to == H8) state.disableCastling(SIDE_BLACK, KING);

        illegalAttackSquares |= bitboards[state.activeColor][KING];
        state.activeColor = OPPOSITE_SIDE(state.activeColor);

        // TEMPORARY move legality check
        MoveList nextMoves;
        getMoves<MoveType::NOISY>(*this, nextMoves);

        for (const Move& move : nextMoves) {
            if (getMask(move.to) & illegalAttackSquares) {
                this->unmakeMove(unmakeMove);
                return INVALID_MOVE;
            }
        }

        return unmakeMove;
    }

    void Board::unmakeMove(const UnmakeMove& unmakeMove) {
        state = unmakeMove.state;

        if (unmakeMove.move == NULL_MOVE) return;

        removePiece(state.activeColor, unmakeMove.move.pieceType, unmakeMove.move.to);
        putPiece(state.activeColor, unmakeMove.move.pieceType, unmakeMove.move.from);

        // castling spaghetti^-1
        if (unmakeMove.move.pieceType == KING && (unmakeMove.move.from == E1 || unmakeMove.move.from == E8)) {
            if (unmakeMove.move.to == C1 || unmakeMove.move.to == G1) { // white
                if (unmakeMove.move.to == C1) {
                    putPiece(SIDE_WHITE, ROOK, A1);
                    removePiece(SIDE_WHITE, ROOK, D1);
                } else {
                    putPiece(SIDE_WHITE, ROOK, H1);
                    removePiece(SIDE_WHITE, ROOK, F1);
                }
            } else if (unmakeMove.move.to == C8 || unmakeMove.move.to == G8) {
                if (unmakeMove.move.to == C8) {
                    putPiece(SIDE_BLACK, ROOK, A8);
                    removePiece(SIDE_BLACK, ROOK, D8);
                } else {
                    putPiece(SIDE_BLACK, ROOK, H8);
                    removePiece(SIDE_BLACK, ROOK, F8);
                }
            }
        }

        // capture
        if (IS_VALID_PIECE(unmakeMove.pieceTaken)) {
            putPiece(OPPOSITE_SIDE(state.activeColor), unmakeMove.pieceTaken, unmakeMove.move.to);
        }

        // promotion
        if (IS_VALID_PIECE(unmakeMove.move.promotionType)) {
            removePiece(state.activeColor, unmakeMove.move.promotionType, unmakeMove.move.to);
        }

        // en passant
        if (unmakeMove.move.pieceType == PAWN && unmakeMove.move.to == state.enpassantSquare) {
            int offset = (state.activeColor == SIDE_WHITE) ? 8 : -8;
            putPiece(OPPOSITE_SIDE(state.activeColor), PAWN, state.enpassantSquare - offset);
        }
    }

    Board& Board::operator=(const Board& other) {
        std::memcpy(bitboards, other.bitboards, sizeof(bitboards));
        occupiedSquares[SIDE_WHITE] = other.occupiedSquares[SIDE_WHITE];
        occupiedSquares[SIDE_BLACK] = other.occupiedSquares[SIDE_BLACK];
        state = other.state;
        return *this;
    }

    std::string indexToPrettyString(uint8_t index) {
        return std::string(1, (index % 8) + 'a') + std::to_string(index / 8 + 1);
    }
    
    // prints the bitboard from black's perspective
    std::string bitboardToPrettyString(uint64_t bitboard) {
        std::ostringstream oss;
        for (int rank = 0; rank < 8; rank++) {
            for (int file = 7; file >= 0; file--) {
                uint64_t mask = choco::getMask(rank, file);
                if (bitboard & mask) oss << "o";
                else oss << "-";
                oss << " ";
            }

            oss << (char) ('1' + rank) << "\n";
        }
        for (int file = 7; file >= 0; file--) {
            oss << (char) ('A' + file)  << " ";
        }
        return oss.str();
    }

    std::string boardToPrettyString(const Board& board) {
        std::ostringstream oss;
        for (int rank = 0; rank < 8; rank++) {
            for (int file = 7; file >= 0; file--) {
                // idk anymore
                     if (board.bitboards[SIDE_WHITE][KING] & getMask(rank, file))   oss << "K ";
                else if (board.bitboards[SIDE_WHITE][QUEEN] & getMask(rank, file))  oss << "Q ";
                else if (board.bitboards[SIDE_WHITE][BISHOP] & getMask(rank, file)) oss << "B ";
                else if (board.bitboards[SIDE_WHITE][KNIGHT] & getMask(rank, file)) oss << "N ";
                else if (board.bitboards[SIDE_WHITE][ROOK] & getMask(rank, file))   oss << "R ";
                else if (board.bitboards[SIDE_WHITE][PAWN] & getMask(rank, file))   oss << "P ";

                else if (board.bitboards[SIDE_BLACK][KING] & getMask(rank, file))   oss << "k ";
                else if (board.bitboards[SIDE_BLACK][QUEEN] & getMask(rank, file))  oss << "q ";
                else if (board.bitboards[SIDE_BLACK][BISHOP] & getMask(rank, file)) oss << "b ";
                else if (board.bitboards[SIDE_BLACK][KNIGHT] & getMask(rank, file)) oss << "n ";
                else if (board.bitboards[SIDE_BLACK][ROOK] & getMask(rank, file))   oss << "r ";
                else if (board.bitboards[SIDE_BLACK][PAWN] & getMask(rank, file))   oss << "p ";
                
                else oss << ". ";
            }
            oss << (char) ('1' + rank) << "\n";
        }
        for (int file = 7; file >= 0; file--) {
            oss << char('A' + file) << " ";
        }
        std::string str = oss.str();
        return str.erase(str.length() - 1);
    }

    std::string pieceToPrettyString(uint8_t piece) {
        switch (piece) {
            case KING: return "King";
            case QUEEN: return "Queen";
            case BISHOP: return "Bishop";
            case KNIGHT: return "Knight";
            case ROOK: return "Rook";
            case PAWN: return "Pawn";
        }

        throw std::invalid_argument("Invalid piece " + std::to_string(piece));
    }
} // namespace choco
