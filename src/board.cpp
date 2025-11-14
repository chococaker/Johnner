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

namespace choco {
    const UnmakeMove INVALID_MOVE = {{0, 0, 100}, 100, {}};

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
            res.push_back(s.substr(pos_start));
            return res;
        }
        
        uint8_t countTrailingZeros(uint64_t n) {
            if (n == 0) {
                return 0;
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

        void addOffsetExtractedMoves(uint64_t bitboard, uint8_t piece, uint8_t offset, std::vector<Move>& moveVec) {
            if (offset > 0) {
                while (bitboard) {
                    uint8_t index = countTrailingZeros(bitboard);
                    bitboard &= ~(1ULL << index);
                    Move move = { piece, index - offset, index };
                    moveVec.push_back(move);
                }
            }
        }

        void addOriginExtractedMoves(uint64_t bitboard, uint8_t piece, uint8_t originIndex, std::vector<Move>& moveVec) {
            while (bitboard != 0) {
                uint8_t index = countTrailingZeros(bitboard);
                bitboard &= ~(1ULL << index);
                Move move = { piece, originIndex, index };
                moveVec.push_back(move);
            }
        }

        // Carry-Ripple Trick
        void enumerateSubsets(uint64_t bitboard, const std::function<void(uint64_t)>& func) {
            uint64_t sub = 0;
            do {
                func(sub);
                sub = (sub - bitboard) & bitboard;
            } while (sub);
        }

        // creates a bitboard with a line of 1s by stepping until it encounters a 1 (inclusive) or border on the parameter bitboard
        uint64_t walk(uint8_t fileStep, uint8_t rankStep, uint64_t occupiedBoard, uint8_t originIndex) { // Renamed 'bitboard' to 'occupiedBoard' for clarity
            int8_t rank = getRank(originIndex);
            int8_t file = getFile(originIndex);
            uint64_t attacks = 0;
            rank += rankStep;
            file += fileStep;
            while (rank >= 0 && rank < 8 && file >= 0 && file < 8) {
                uint8_t index = getIndex(rank, file);
                uint64_t mask = getMask(index);
                attacks |= mask;
                // if the current square is occupied, stop walk
                if (occupiedBoard & mask) break;
                rank += rankStep;
                file += fileStep;
            }
            return attacks;
        }
    
        uint64_t shiftLeftBasedOnColor(uint8_t color, uint64_t val, uint8_t amount) {
            if (color == SIDE_WHITE) return val << amount;
            else return val >> amount;
        }
    }

    struct Magic {
        uint64_t mask;
        uint64_t magic;
    };

    uint64_t ROOK_SEED   = 459371994;
    uint64_t BISHOP_SEED = 2595412012;

    // "plain" approach https://www.chessprogramming.org/Magic_Bitboards
    Magic ROOK_TBL[64] = {0};
    uint64_t ROOK_ATTACKS[64][4096] = {0};

    Magic BISHOP_TBL[64] = {0};
    uint64_t BISHOP_ATTACKS[64][512] = {0};

    uint64_t KNIGHT_ATTACKS[64] = {0};
    uint64_t KING_ATTACKS[64] = {0};

    template<size_t shifts>
    uint64_t initMagics(
        uint64_t seed,
        Magic table[64],
        uint64_t attacks[64][1 << shifts],
        const std::function<uint64_t(uint64_t bitboard, uint8_t index)>& attackGenerator,
        const std::function<uint64_t(uint8_t index)>& maskGenerator
    ) {
        std::mt19937_64 engine(seed);

        uint64_t iterations = 0;

        uint16_t attacksSize = 1 << shifts;

        std::memset(attacks, 0, 64 * (1 << shifts) * sizeof(uint64_t));
        std::memset(table, 0, 64 * sizeof(Magic));
        
        for (int i = 0; i < 64; i++) {
            Magic& val = table[i];

            // mask
            val.mask = maskGenerator(i);

            // bitboard generation
            uint64_t _look[1 << shifts] = {0};
            int _lookIndex = 0;
            enumerateSubsets(val.mask, [&_look, &_lookIndex, &attackGenerator, i](uint64_t bitboard) -> void {
                _look[_lookIndex++] = attackGenerator(bitboard, i);
            });

            // magic
            bool validMagic = true;
            while (true) {
                // reset
                std::memset(attacks[i], 0, sizeof(uint64_t) * attacksSize);
                validMagic = true;
                val.magic = engine() & engine() & engine();
                _lookIndex = 0; // reuse the old variable why not

                int magicCheckCount_ = 0;

                enumerateSubsets(val.mask, [&validMagic,
                                            &val,
                                            &_look,
                                            &_lookIndex,
                                            &magicCheckCount_,
                                            &iterations,
                                            &attacks,
                                            i](uint64_t bitboard) -> void {
                    if (!validMagic) return; // skip through iterations if magic num is invalid

                    iterations++;

                    // hash, then isolate last digits
                    uint16_t hash = ((bitboard * val.magic) >> (64 - shifts)) & ((1ULL << shifts) - 1);

                    uint64_t attackBoard = _look[_lookIndex++];
                    magicCheckCount_++;

                    if (attacks[i][hash] && attacks[i][hash] != attackBoard) { // hash collision
                        validMagic = false;
                        return;
                    }
                    
                    attacks[i][hash] = attackBoard;
                });

                if (validMagic) break;
            }
        }

        return iterations;
    }

    void initKingBoards() {
        static auto addIfInBounds = [](uint64_t& mask, int8_t rank, int8_t file) -> void {
            if (rank >= 0 && file >= 0 && rank < 8 && file < 8) {
                mask |= getMask(getIndex(rank, file));
            }
        };

        for (int i = 0; i < 64; i++) {
            uint64_t mask = 0;

            int8_t rank = static_cast<int8_t>(getRank(i));
            int8_t file = static_cast<int8_t>(getFile(i));

            // really dirty but idc
            // only run once
            addIfInBounds(mask, rank - 1, file - 1);
            addIfInBounds(mask, rank - 1, file + 0);
            addIfInBounds(mask, rank - 1, file + 1);
            addIfInBounds(mask, rank + 0, file + 1);
            addIfInBounds(mask, rank + 1, file + 1);
            addIfInBounds(mask, rank + 1, file - 0);
            addIfInBounds(mask, rank + 1, file - 1);
            addIfInBounds(mask, rank - 0, file - 1);

            KING_ATTACKS[i] = mask;
        }
    }

    void initKnightBoards() {
        static auto addIfInBounds = [](uint64_t& mask, int8_t rank, int8_t file) -> void {
            if (rank >= 0 && file >= 0 && rank < 8 && file < 8) {
                mask |= getMask(getIndex(rank, file));
            }
        };

        for (int i = 0; i < 64; i++) {
            uint64_t mask = 0;

            int8_t rank = static_cast<int8_t>(getRank(i));
            int8_t file = static_cast<int8_t>(getFile(i));

            // really dirty but idc
            // only run once
            addIfInBounds(mask, rank - 2, file - 1);
            addIfInBounds(mask, rank - 2, file + 1);
            addIfInBounds(mask, rank - 1, file + 2);
            addIfInBounds(mask, rank + 1, file + 2);
            addIfInBounds(mask, rank + 2, file + 1);
            addIfInBounds(mask, rank + 2, file - 1);
            addIfInBounds(mask, rank + 1, file - 2);
            addIfInBounds(mask, rank - 1, file - 2);

            KNIGHT_ATTACKS[i] = mask;
        }
    }

    uint64_t initBishopBoards(uint64_t seed) {
        static std::function<uint64_t(uint64_t, uint8_t)> attackGenerator = [](uint64_t bitboard, uint8_t index) -> uint64_t {
            uint64_t attackBitboard = walk(1, 1, bitboard, index)
                                    | walk(1, -1, bitboard, index)
                                    | walk(-1, 1, bitboard, index)
                                    | walk(-1, -1, bitboard, index);
            return attackBitboard;
        };

        static std::function<uint64_t(uint8_t)> maskGenerator = [](uint8_t index) -> uint64_t {
            uint64_t mask = walk(1, 1, 0, index)
                          | walk(1, -1, 0, index)
                          | walk(-1, 1, 0, index)
                          | walk(-1, -1, 0, index);

            mask &= (~BITBOARD_FILE_A);
            mask &= (~BITBOARD_FILE_H);
            mask &= (~BITBOARD_RANK_1);
            mask &= (~BITBOARD_RANK_8);

            mask &= ~(choco::getMask(index));

            return mask;
        };

        return initMagics<9>(seed, BISHOP_TBL, BISHOP_ATTACKS, attackGenerator, maskGenerator);
    }

    uint64_t initRookBoards(uint64_t seed) {
        static std::function<uint64_t(uint64_t, uint8_t)> attackGenerator = [](uint64_t bitboard, uint8_t index) -> uint64_t {
            uint64_t attackBitboard = walk(1, 0, bitboard, index)
                                    | walk(-1, 0, bitboard, index)
                                    | walk(0, 1, bitboard, index)
                                    | walk(0, -1, bitboard, index);
            
            return attackBitboard;
        };

        static std::function<uint64_t(uint8_t)> maskGenerator = [](uint8_t index) -> uint64_t {
            uint64_t fileMask = choco::getFileMask(index % 8);
            uint64_t rankMask = choco::getRankMask(index / 8);
            uint64_t mask = fileMask | rankMask;

            if (fileMask != BITBOARD_FILE_A) mask &= (~BITBOARD_FILE_A);
            if (fileMask != BITBOARD_FILE_H) mask &= (~BITBOARD_FILE_H);
            if (rankMask != BITBOARD_RANK_1) mask &= (~BITBOARD_RANK_1);
            if (rankMask != BITBOARD_RANK_8) mask &= (~BITBOARD_RANK_8);

            mask &= ~(choco::getMask(index));

            return mask;
        };

        return initMagics<12>(seed, ROOK_TBL, ROOK_ATTACKS, attackGenerator, maskGenerator);
    }

    // returns number of attempts required to generate magics
    void initBitboards() {
        std::cout << "Initializing bitboards:" << std::endl;

        std::cout << "  > Initializing rook moves...   ";
        uint64_t rookIterations = initRookBoards(ROOK_SEED);
        std::cout << "Done (" << rookIterations << " iterations)" << std::endl;

        std::cout << "  > Initializing bishop moves... ";
        uint64_t bishopIterations = initBishopBoards(BISHOP_SEED);
        std::cout << "Done (" << bishopIterations << " iterations)" << std::endl;

        std::cout << "  > Initializing knight moves... ";
        initKnightBoards();
        std::cout << "Done" << std::endl;

        std::cout << "  > Initializing king moves...   ";
        initKingBoards();
        std::cout << "Done" << std::endl;

        std::cout << "  > Finished all." << std::endl;
    }

    bool Move::operator==(const Move& other) {
        return pieceType == other.pieceType
                && from == other.from
                && to == other.to
                && promotionType == other.promotionType;
    }

    bool UnmakeMove::isValid() const {
        return IS_VALID_PIECE(move.pieceType);
    }

    void GameState::enableCastling(uint8_t color, uint8_t sidePiece) {
        uint8_t mask = 1 << ((sidePiece - KING) + color * 2);
        castling |= mask;
    }

    void GameState::disableCastling(uint8_t color, uint8_t sidePiece) {
        uint8_t mask = 1 << ((sidePiece - KING) + color * 2);
        castling &= ~mask;
    }

    bool GameState::canCastle(uint8_t color, uint8_t sidePiece) const {
        uint8_t mask = 1 << ((sidePiece - KING) + color * 2);
        return (castling & mask) != 0;
    }

    Board::Board() {
        memset(bitboards, 0, sizeof(bitboards));
        state = { SIDE_WHITE, 0, 0, 100, 0 };
    }

    Board::Board(const std::string& fen) : Board() {
        std::vector<std::string> fenParts = split(fen, " ");
        
        // piece positions
        std::vector<std::string> positions = split(fenParts[0], "/");
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
        state.halfMoveClock = std::stoi(fenParts[4]);
        
        // full moves
        state.moveCount = std::stoi(fenParts[5]);
    }

    void Board::putPiece(uint8_t side, uint8_t piece, uint8_t index) {
        bitboards[side][piece] |= (1ULL << index);
    }
    void Board::removePiece(uint8_t side, uint8_t piece, uint8_t index) {
        bitboards[side][piece] &= ~getMask(index);
    }
    UnmakeMove Board::makeMove(const Move& move) {
        UnmakeMove unmakeMove = { .move = move, .pieceTaken = 127, .state = state };

        removePiece(state.activeColor, move.pieceType, move.from);
        putPiece(state.activeColor, move.pieceType, move.to);

        state.halfMoveClock++;

        // capture
        for (uint8_t i = 0; i < 6; i++) {
            uint64_t initialVal = bitboards[OPPOSITE_SIDE(state.activeColor)][i];
            bitboards[OPPOSITE_SIDE(state.activeColor)][i] &= ~getMask(move.to);
            if (initialVal != bitboards[OPPOSITE_SIDE(state.activeColor)][i]) {
                state.halfMoveClock = 0;
                unmakeMove.pieceTaken = i;
                break;
            }
        }

        uint64_t illegalAttackSquares = 0; // squares that are not allowed be attacked if this move is played

        if (move.pieceType == PAWN) {
            state.halfMoveClock = 0;

            // en passant
            if (move.to == state.enpassantSquare) {
                int offset = (state.activeColor == SIDE_WHITE) ? 8 : -8;
                removePiece(OPPOSITE_SIDE(state.activeColor), PAWN, state.enpassantSquare - offset);
            }

            state.enpassantSquare = 127;

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

        // lazy move legality check. does not check for king attack as engine will avoid moves like that at all costs
        if (illegalAttackSquares & getAttacks(state.activeColor)) {
            this->unmakeMove(unmakeMove);
            return INVALID_MOVE;
        }

        return unmakeMove;
    }

    void Board::unmakeMove(const UnmakeMove& unmakeMove) {
        this->state = unmakeMove.state;
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

    std::vector<Move> Board::generatePLMoves() const {
        std::vector<Move> moves;
        addPawnMoves(moves);
        addBishopMoves(moves);
        addKnightMoves(moves);
        addQueenMoves(moves);
        addRookMoves(moves);
        addKingMoves(moves);
        return moves;
    }

    void Board::addKingMoves(std::vector<Move>& moves) const {
        iterateIndices(bitboards[state.activeColor][KING], [this, &moves](uint8_t index) -> void {
            addOriginExtractedMoves(plKingMoveBB(index, state.activeColor), KING, index, moves);
        });

        // castling spaghetti
        if (state.canCastle(state.activeColor, KING)) {
            uint64_t occupied = getOccupiedBitboard(bitboards);
            uint64_t mask = state.activeColor == SIDE_WHITE ? F1 | G1 : F8 | G8;
            occupied &= mask;
            if (!occupied) moves.push_back(state.activeColor == SIDE_WHITE ? Move(KING, E1, G1) : Move(KING, E8, G8));
        }
        if (state.canCastle(state.activeColor, QUEEN)) {
            uint64_t occupied = getOccupiedBitboard(bitboards);
            uint64_t mask = (state.activeColor == SIDE_WHITE)
                            ? getMask(D1) | getMask(C1) | getMask(B1)
                            : getMask(D8) | getMask(C8) | getMask(B8);
            occupied &= mask;
            if (!occupied) moves.push_back(state.activeColor == SIDE_WHITE ? Move(KING, E1, C1) : Move(KING, E8, C8));
        }
    }

    void Board::addQueenMoves(std::vector<Move>& moves) const {
        iterateIndices(bitboards[state.activeColor][QUEEN], [this, &moves](uint8_t index) -> void {
            addOriginExtractedMoves(plQueenMoveBB(index, state.activeColor), QUEEN, index, moves);
        });
    }

    void Board::addKnightMoves(std::vector<Move>& moves) const {
        iterateIndices(bitboards[state.activeColor][KNIGHT], [this, &moves](uint8_t index) -> void {
            addOriginExtractedMoves(plKnightMoveBB(index, state.activeColor), KNIGHT, index, moves);
        });
    }

    void Board::addBishopMoves(std::vector<Move>& moves) const {
        iterateIndices(bitboards[state.activeColor][BISHOP], [this, &moves](uint8_t index) -> void {
            addOriginExtractedMoves(plBishopMoveBB(index, state.activeColor), BISHOP, index, moves);
        });
    }

    void Board::addRookMoves(std::vector<Move>& moves) const {
        iterateIndices(bitboards[state.activeColor][ROOK], [this, &moves](uint8_t index) -> void {
            addOriginExtractedMoves(plRookMoveBB(index, state.activeColor), ROOK, index, moves);
        });
    }

    void Board::addPawnMoves(std::vector<Move>& moves) const {
        int shiftFactor = state.activeColor == SIDE_WHITE ? 1 : -1;

        if (!bitboards[state.activeColor][PAWN]) return;

        uint8_t color = state.activeColor;

        // pushes
        uint64_t emptySquares = getEmptyBitboard(bitboards);
        uint64_t promoterMask = (color == SIDE_WHITE) ? BITBOARD_RANK_7 : BITBOARD_RANK_2;
        uint64_t pushedPawns = shiftLeftBasedOnColor(color, bitboards[color][PAWN] & ~promoterMask, 8) & emptySquares;
        addOffsetExtractedMoves(pushedPawns, PAWN, 8 * shiftFactor, moves);

        // double pushes
        uint64_t doublePushRankMask = (color == SIDE_WHITE) ? BITBOARD_RANK_3 : BITBOARD_RANK_6;
        uint64_t doublePushedPawns = shiftLeftBasedOnColor(color, pushedPawns & doublePushRankMask, 8) & emptySquares;
        addOffsetExtractedMoves(doublePushedPawns, PAWN, 16 * shiftFactor, moves);

        // captures
        uint64_t oppSquares = getOccupiedBitboard(bitboards[OPPOSITE_SIDE(color)]);
        if (state.enpassantSquare < 64) oppSquares |= getMask(state.enpassantSquare);
        uint64_t peripheralPawnsL = (color == SIDE_WHITE) ? BITBOARD_FILE_H : BITBOARD_FILE_A;
        uint64_t peripheralPawnsR = (color == SIDE_WHITE) ? BITBOARD_FILE_A : BITBOARD_FILE_H;
        uint64_t captureLPawns = shiftLeftBasedOnColor(color, bitboards[color][PAWN] & ~peripheralPawnsR, 7) & oppSquares;
        uint64_t captureRPawns = shiftLeftBasedOnColor(color, bitboards[color][PAWN] & ~peripheralPawnsL, 9) & oppSquares;
        addOffsetExtractedMoves(captureLPawns, PAWN, 7 * shiftFactor, moves);
        addOffsetExtractedMoves(captureRPawns, PAWN, 9 * shiftFactor, moves);

        // promotion
        uint64_t promoters = bitboards[color][PAWN] & promoterMask;
        while (promoters != 0) {
            uint8_t index = countTrailingZeros(promoters);
            promoters &= ~(1ULL << index);
            uint8_t promotionSq = index + 8 * shiftFactor;
            if (getMask(promotionSq) & emptySquares || getMask(promotionSq) & oppSquares) {
                moves.push_back({PAWN, index, promotionSq, QUEEN});
                moves.push_back({PAWN, index, promotionSq, KNIGHT});
                moves.push_back({PAWN, index, promotionSq, ROOK});
                moves.push_back({PAWN, index, promotionSq, BISHOP});
            }
        }
    }

    uint8_t Board::countPieces(uint8_t side, uint8_t piece) const {
        uint64_t n = bitboards[side][piece];
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_popcountll(n);
#elif defined(_MSC_VER)
        #include <intrin.h>
        return __popcount64(n);
#else
        uint8_t count = 0;
        while (n > 0) {
            n &= (n - 1); 
            count++;
        }
        return count;
#endif
    }

    uint64_t Board::getAttacks(uint8_t color) const {
        uint64_t attacks = 0;
        iterateIndices(bitboards[color][KING], [this, color, &attacks](uint8_t index) -> void {
            attacks |= plKingMoveBB(index, color);
        });
        iterateIndices(bitboards[color][QUEEN], [this, color, &attacks](uint8_t index) -> void {
            attacks |= plQueenMoveBB(index, color);
        });
        iterateIndices(bitboards[color][KNIGHT], [this, color, &attacks](uint8_t index) -> void {
            attacks |= plKnightMoveBB(index, color);
        });
        iterateIndices(bitboards[color][BISHOP], [this, color, &attacks](uint8_t index) -> void {
            attacks |= plBishopMoveBB(index, color);
        });
        iterateIndices(bitboards[color][ROOK], [this, color, &attacks](uint8_t index) -> void {
            attacks |= plRookMoveBB(index, color);
        });

        // pawns
        uint64_t peripheralPawnsL = (color == SIDE_WHITE) ? BITBOARD_FILE_H : BITBOARD_FILE_A;
        uint64_t peripheralPawnsR = (color == SIDE_WHITE) ? BITBOARD_FILE_A : BITBOARD_FILE_H;
        uint64_t captureLPawns = shiftLeftBasedOnColor(color, bitboards[color][PAWN] & ~peripheralPawnsR, 7);
        uint64_t captureRPawns = shiftLeftBasedOnColor(color, bitboards[color][PAWN] & ~peripheralPawnsL, 9);
        attacks |= captureLPawns;
        attacks |= captureRPawns;
        
        return attacks;
    }

    uint64_t Board::plKingMoveBB(uint8_t square, uint8_t color) const {
        return KING_ATTACKS[square] & (~getOccupiedBitboard(bitboards[color]));
    }

    uint64_t Board::plQueenMoveBB(uint8_t square, uint8_t color) const {
        uint64_t occupiedBitboard = getOccupiedBitboard(bitboards);

        const Magic& rookVal = ROOK_TBL[square];
        uint64_t rookRelevantBitboard = occupiedBitboard & rookVal.mask;
        uint16_t rookHash = ((rookRelevantBitboard * rookVal.magic) >> (64 - 12)) & ((1ULL << 12) - 1);
        uint64_t rookStuff = ROOK_ATTACKS[square][rookHash] & (~getOccupiedBitboard(bitboards[color]));

        const Magic& bishopVal = BISHOP_TBL[square];
        uint64_t bishopRelevantBitboard = occupiedBitboard & bishopVal.mask;
        uint16_t bishopHash = ((bishopRelevantBitboard * bishopVal.magic) >> (64 - 9)) & ((1ULL << 9) - 1);
        uint64_t bishopStuff = BISHOP_ATTACKS[square][bishopHash] & (~getOccupiedBitboard(bitboards[color]));

        return rookStuff | bishopStuff;
    }

    uint64_t Board::plBishopMoveBB(uint8_t square, uint8_t color) const {
        const Magic& val = BISHOP_TBL[square];
        uint64_t relevantBitboard = getOccupiedBitboard(bitboards) & val.mask;
        uint16_t hash = ((relevantBitboard * val.magic) >> (64 - 9)) & ((1ULL << 9) - 1);
        return BISHOP_ATTACKS[square][hash] & (~getOccupiedBitboard(bitboards[color]));
    }

    uint64_t Board::plKnightMoveBB(uint8_t square, uint8_t color) const {
        return KNIGHT_ATTACKS[square] & (~getOccupiedBitboard(bitboards[color]));
    }
    
    uint64_t Board::plRookMoveBB(uint8_t square, uint8_t color) const {
        const Magic& val = ROOK_TBL[square];
        uint64_t relevantBitboard = getOccupiedBitboard(bitboards) & val.mask;
        uint16_t hash = ((relevantBitboard * val.magic) >> (64 - 12)) & ((1ULL << 12) - 1);
        return ROOK_ATTACKS[square][hash] & (~getOccupiedBitboard(bitboards[color]));
    }
    
    uint64_t getMask(uint8_t index) {
        return 1ULL << index;
    }

    uint64_t getMask(int rank, int file) {
        return getMask(getIndex(rank, file));
    }

    uint64_t getRankMask(int rank) {
        switch (rank) {
            case 0: return BITBOARD_RANK_1;
            case 1: return BITBOARD_RANK_2;
            case 2: return BITBOARD_RANK_3;
            case 3: return BITBOARD_RANK_4;
            case 4: return BITBOARD_RANK_5;
            case 5: return BITBOARD_RANK_6;
            case 6: return BITBOARD_RANK_7;
            case 7: return BITBOARD_RANK_8;
        }

        throw std::invalid_argument("OOB rank " + std::to_string(rank));
    }

    uint64_t getFileMask(int file) {
        switch (file) {
            case 0: return BITBOARD_FILE_A;
            case 1: return BITBOARD_FILE_B;
            case 2: return BITBOARD_FILE_C;
            case 3: return BITBOARD_FILE_D;
            case 4: return BITBOARD_FILE_E;
            case 5: return BITBOARD_FILE_F;
            case 6: return BITBOARD_FILE_G;
            case 7: return BITBOARD_FILE_H;
        }

        throw std::invalid_argument("OOB file " + std::to_string(file));
    }

    uint8_t getIndex(int rank, int file) {
        return (rank * 8) + file;
    }

    uint8_t getRank(uint8_t index) {
        return index / 8;
    }

    uint8_t getFile(uint8_t index) {
        return index % 8;
    }

    std::string indexToPrettyString(uint8_t index) {
        return std::string(1, (index % 8) + 'A') + std::to_string(index / 8 + 1);
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

            oss << (char) ('A' + rank) << "\n";
        }
        for (int file = 7; file >= 0; file--) {
            oss << (char) ('1' + file)  << " ";
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
            oss << (char) ('A' + rank) << "\n";
        }
        for (int file = 7; file >= 0; file--) {
            oss << char('1' + file) << " ";
        }
        std::string str = oss.str();
        return str.erase(str.length() - 1);
    }


    void iterateIndices(uint64_t bitboard, const std::function<void(uint8_t)>& func) {
        while (bitboard) {
            uint8_t index = countTrailingZeros(bitboard);
            bitboard &= ~getMask(index);
            func(index);
        }
    }

    uint64_t getOccupiedBitboard(const uint64_t bitboards[6]) {
        uint64_t bitboard = 0;
        for (size_t i = 0; i < 6; i++) {
            bitboard |= bitboards[i];
        }
        return bitboard;
    }

    uint64_t getOccupiedBitboard(const uint64_t bitboards[2][6]) {
        return getOccupiedBitboard(bitboards[0]) | getOccupiedBitboard(bitboards[1]);
    }

    uint64_t getEmptyBitboard(const uint64_t bitboards[2][6]) {
        return ~getOccupiedBitboard(bitboards);
    }
} // namespace choco
