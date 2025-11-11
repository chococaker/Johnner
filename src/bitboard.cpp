#include "bitboard.h"

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

        void addOffsetExtractedMoves(uint64_t bitboard, uint8_t offset, std::vector<Move>& moveVec) {
            while (bitboard != 0) {
                uint8_t index = countTrailingZeros(bitboard);
                bitboard &= ~(1ULL << index);
                Move move = { index - offset, index };
                moveVec.push_back(move);
            }
        }

        void addOriginExtractedMoves(uint64_t bitboard, uint8_t originIndex, std::vector<Move>& moveVec) {
            while (bitboard != 0) {
                uint8_t index = countTrailingZeros(bitboard);
                bitboard &= ~(1ULL << index);
                Move move = { originIndex, index };
                moveVec.push_back(move);
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

        // Carry-Ripple Trick
        void enumerateSubsets(uint64_t bitboard, std::function<void(uint64_t)> func) {
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
    }

    struct Magic {
        uint64_t mask;
        uint64_t magic;
    };

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
            uint64_t $look[attacksSize] = {0};
            int $lookIndex = 0;
            enumerateSubsets(val.mask, [&$look, &$lookIndex, &attackGenerator, i](uint64_t bitboard) -> void {
                $look[$lookIndex++] = attackGenerator(bitboard, i);
            });

            // magic
            bool validMagic = true;
            while (true) {
                // reset
                std::memset(attacks[i], 0, sizeof(uint64_t) * attacksSize);
                validMagic = true;
                val.magic = engine() & engine() & engine();
                $lookIndex = 0; // reuse the old variable why not

                int magicCheckCount_ = 0;

                enumerateSubsets(val.mask, [&validMagic,
                                            &val,
                                            &$look,
                                            &$lookIndex,
                                            &magicCheckCount_,
                                            &iterations,
                                            &attacks,
                                            i](uint64_t bitboard) -> void {
                    if (!validMagic) return; // skip through iterations if magic num is invalid

                    iterations++;

                    // hash, then isolate last digits
                    uint16_t hash = ((bitboard * val.magic) >> (64 - shifts)) & ((1ULL << shifts) - 1);

                    uint64_t attackBoard = $look[$lookIndex++];
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

    // returns number of attempts required to generate magics
    void initBitboards(uint64_t rookSeed, uint64_t bishopSeed) {
        std::cout << "Initializing bitboards:" << std::endl;

        std::cout << "  > Initializing rook moves..." << std::endl;
        uint64_t rookIterations = initRookBoards(rookSeed);
        std::cout << "    Finished (" << rookIterations << " iterations)" << std::endl;

        std::cout << "  > Initializing bishop moves..." << std::endl;
        uint64_t bishopIterations = initBishopBoards(bishopSeed);
        std::cout << "    Finished (" << bishopIterations << " iterations)" << std::endl;

        std::cout << "  > Initializing knight moves..." << std::endl;
        initKnightBoards();
        std::cout << "    Finished" << std::endl;

        std::cout << "  > Finished all." << std::endl;
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

    void Board::putPiece(uint8_t side, uint8_t piece, uint8_t index) {
        bitboards[side][piece] |= (1ULL << index);
    }
    void Board::removePiece(uint8_t side, uint8_t piece, uint8_t index) {
        bitboards[side][piece] ^= (1ULL << index);
    }
    void Board::makeMove(const Move& move, uint8_t piece) {
        removePiece(state.activeColor, piece, move.from);
        putPiece(state.activeColor, piece, move.to);

        state.activeColor = !state.activeColor;

        // double push handling
        if (piece == PAWN && (move.from - move.to == 16 || move.to - move.from == 16)) {
            if (move.from - move.to == 16) {
                state.enpassantSquare = move.from - 8;
            } else if (move.to - move.from == 16) {
                state.enpassantSquare = move.from + 8;
            } else {
                state.enpassantSquare = 127; // impossible
            }
        }

        // en passant handling
        if (move.to == state.enpassantSquare) {
            int offset = state.activeColor == SIDE_WHITE ? -8 : 8;
            removePiece(OPPOSITE_SIDE(state.activeColor), PAWN, move.to + offset);
        }
    }

    std::vector<Move> Board::generateWhiteKnightMoves() const {
        std::vector<Move> moves;

        uint64_t knightBoard = bitboards[SIDE_WHITE][KNIGHT];
        uint64_t occupiedBitboard = getOccupiedBitboard(bitboards);
        while (knightBoard) {
            uint8_t knightIndex = countTrailingZeros(knightBoard);
            knightBoard &= ~getMask(knightIndex);
            uint64_t stuff = KNIGHT_ATTACKS[knightIndex] &= (~getOccupiedBitboard(bitboards[SIDE_WHITE]));
            addOriginExtractedMoves(stuff, knightIndex, moves);
        }

        return moves;
    }

    std::vector<Move> Board::generateWhiteBishopMoves() const {
        std::vector<Move> moves;

        uint64_t bishopBoard = bitboards[SIDE_WHITE][BISHOP];
        uint64_t occupiedBitboard = getOccupiedBitboard(bitboards);
        while (bishopBoard) {
            uint8_t bishopIndex = countTrailingZeros(bishopBoard);
            bishopBoard &= ~getMask(bishopIndex);
            const Magic& val = BISHOP_TBL[bishopIndex];
            uint64_t relevantBitboard = occupiedBitboard & val.mask;
            uint16_t hash = ((relevantBitboard * val.magic) >> (64 - 9)) & ((1ULL << 9) - 1);
            uint64_t stuff = BISHOP_ATTACKS[bishopIndex][hash] & (~getOccupiedBitboard(bitboards[SIDE_WHITE]));
            addOriginExtractedMoves(stuff, bishopIndex, moves);
        }

        return moves;
    }

    std::vector<Move> Board::generateWhiteRookMoves() const {
        std::vector<Move> moves;

        uint64_t rookBoard = bitboards[SIDE_WHITE][ROOK];
        uint64_t occupiedBitboard = getOccupiedBitboard(bitboards);
        while (rookBoard) {
            uint8_t rookIndex = countTrailingZeros(rookBoard);
            rookBoard &= ~getMask(rookIndex);
            const Magic& val = ROOK_TBL[rookIndex];
            uint64_t relevantBitboard = occupiedBitboard & val.mask;
            uint16_t hash = ((relevantBitboard * val.magic) >> (64 - 12)) & ((1ULL << 12) - 1);
            uint64_t stuff = ROOK_ATTACKS[rookIndex][hash] & (~getOccupiedBitboard(bitboards[SIDE_WHITE]));
            addOriginExtractedMoves(stuff, rookIndex, moves);
        }

        return moves;
    }

    std::vector<Move> Board::generateWhitePawnMoves() const {
        if (!bitboards[SIDE_WHITE][PAWN]) return std::vector<Move>();

        std::vector<Move> moves;

        // pushes
        uint64_t emptySquares = getEmptyBitboard(bitboards);
        uint64_t pushedPawns = ((bitboards[SIDE_WHITE][PAWN] & ~BITBOARD_RANK_7) << 8) & emptySquares;
        uint64_t doublePushedPawns = ((pushedPawns & BITBOARD_RANK_3) << 8) & emptySquares;
        addOffsetExtractedMoves(pushedPawns, 8, moves);
        addOffsetExtractedMoves(doublePushedPawns, 16, moves);

        // captures
        uint64_t blackSquares = getOccupiedBitboard(bitboards[SIDE_BLACK]);
        uint64_t captureLeftPawns = ((bitboards[SIDE_WHITE][PAWN] & ~BITBOARD_RANK_7) & blackSquares) << 7;
        uint64_t captureRightPawns = ((bitboards[SIDE_WHITE][PAWN] & ~BITBOARD_RANK_7) & blackSquares) << 9;
        addOffsetExtractedMoves(captureLeftPawns, 7, moves);
        addOffsetExtractedMoves(captureRightPawns, 9, moves);

        // en passant
        if (state.enpassantSquare < 64) {
            uint8_t passantLeftPawnIndex = state.enpassantSquare - 9;
            uint8_t passantRightPawnIndex = state.enpassantSquare - 7;
            addOffsetExtractedMoves(bitboards[SIDE_WHITE][PAWN] & getMask(passantLeftPawnIndex), -9, moves);
            addOffsetExtractedMoves(bitboards[SIDE_WHITE][PAWN] & getMask(passantRightPawnIndex), -7, moves);
        }

        return moves;
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

        throw std::invalid_argument("OOB rank " + rank);
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

        throw std::invalid_argument("OOB file " + file);
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
    // H1(7)  --  A1(0)
    // |              |
    // H8(63) -- H1(55)
    std::string bitboardToPrettyString(uint64_t bitboard) {
        std::ostringstream oss;
        for (int rank = 0; rank < 8; rank++) {
            for (int file = 7; file >= 0; file--) {
                uint64_t mask = choco::getMask(rank, file);
                oss << (((bool) (bitboard & mask))) << " ";
            }
            oss << "\n";
        }
        std::string board = oss.str();
        return board.erase(board.length() - 1);
    }
} // namespace choco
