#include "movegen.h"

#include <cstring>
#include <random>

#include "macros.h"
#include "bithelpers.h"
#include "types.h"

namespace choco {
    struct Magic {
        uint64_t mask;
        uint64_t magic;
    };

    constexpr static uint64_t ROOK_SEED   = 459371994;
    constexpr static uint64_t BISHOP_SEED = 2595412012;

    constexpr static uint8_t ROOK_SHIFTS = 12;
    constexpr static size_t ROOK_ATTACKS_SIZE = 1 << ROOK_SHIFTS;
    static Magic ROOK_MAGIC[MAX_SQUARE] = {0};
    static uint64_t ROOK_ATTACKS[MAX_SQUARE][ROOK_ATTACKS_SIZE] = {0};

    constexpr static uint8_t BISHOP_SHIFTS = 9;
    constexpr static  size_t BISHOP_ATTACKS_SIZE = 1 << BISHOP_SHIFTS;
    static Magic BISHOP_MAGIC[MAX_SQUARE] = {0};
    static uint64_t BISHOP_ATTACKS[MAX_SQUARE][BISHOP_ATTACKS_SIZE] = {0};

    static uint64_t KNIGHT_ATTACKS[64] = {0};
    static uint64_t KING_ATTACKS[64] = {0};

    uint64_t getBishopBlockerMask(uint8_t index) {
        uint64_t rays = walk(1, 1, 0, index)
                      | walk(1, -1, 0, index)
                      | walk(-1, 1, 0, index)
                      | walk(-1, -1, 0, index);

        rays &= (~BITBOARD_FILE_A);
        rays &= (~BITBOARD_FILE_H);
        rays &= (~BITBOARD_RANK_1);
        rays &= (~BITBOARD_RANK_8);

        rays &= ~(choco::getMask(index));

        return rays;
    }

    uint64_t getRookBlockerMask(uint8_t index) {
        // retrieve attack rays
        uint64_t rankMask = getRankMask(getRank(index));
        uint64_t fileMask = getFileMask(getFile(index));
        uint64_t rays = (getFileMask(getFile(index)) | getRankMask(getRank(index)));

        // remove edges UNLESS rook is on the edge (for magics)
        if (fileMask != BITBOARD_FILE_A) rays &= (~BITBOARD_FILE_A);
        if (fileMask != BITBOARD_FILE_H) rays &= (~BITBOARD_FILE_H);
        if (rankMask != BITBOARD_RANK_1) rays &= (~BITBOARD_RANK_1);
        if (rankMask != BITBOARD_RANK_8) rays &= (~BITBOARD_RANK_8);

        // exclude the location of the rook
        rays &= ~getMask(index);

        return rays;
    }
    
    template<uint8_t shifts>
    constexpr inline uint16_t getHash(uint64_t bitboard, uint64_t magic) {
        // apply magic, shift right, ensure in-bounds
        return ((bitboard * magic) >> (64 - shifts)) & ((1ULL << shifts) - 1);
    }

    template<uint8_t pieceType>
    void populateMagic() {
        static_assert(pieceType == ROOK || pieceType == BISHOP);

        //                                  seeds: ROOK v      BISHOP v
        constexpr int SEED = (pieceType == ROOK) ? 459371994 : 2595412012;
        constexpr Magic* magicArr = (pieceType == ROOK) ? ROOK_MAGIC : BISHOP_MAGIC;
        constexpr uint16_t attackSize = (pieceType == ROOK) ? ROOK_ATTACKS_SIZE : BISHOP_ATTACKS_SIZE;
        constexpr uint8_t shifts = (pieceType == ROOK) ? ROOK_SHIFTS : BISHOP_SHIFTS;
        constexpr auto getBlockerMask = (pieceType == ROOK) ? getRookBlockerMask : getBishopBlockerMask;
        const auto& attacks = []() {
            if constexpr (pieceType == ROOK) {
                return ROOK_ATTACKS;
            } else if constexpr (pieceType == BISHOP) {
                return BISHOP_ATTACKS;
            }
        }();

        std::mt19937_64 magicGenerator(SEED);

        // enumerate through all squares
        for (size_t i = 0; i < MAX_SQUARE; i++) {
            uint64_t blockers = getBlockerMask(i);
            magicArr[i].mask = blockers;

            while (true) {
                // hash 3x to keep complexity low (i think)
                uint64_t magic = magicGenerator() & magicGenerator() & magicGenerator();
                // reset from previous attempt (garbage)
                std::memset(attacks[i], 0, sizeof(uint64_t) * attackSize);

                bool isValidHash = true;

                // enumerate through all permutations of blocker (Carry-Ripple)
                uint64_t permut = 0;
                do {
                    uint64_t attackBoard;
                    if constexpr (pieceType == ROOK) {
                        // side to side
                        attackBoard = walk(1, 0, permut, i) | walk(-1, 0, permut, i)
                                    | walk(0, 1, permut, i) | walk(0, -1, permut, i);
                    } else if constexpr (pieceType == BISHOP) {
                        // diagonal
                        attackBoard = walk(1, 1, permut, i)  | walk(1, -1, permut, i)
                                    | walk(-1, 1, permut, i) | walk(-1, -1, permut, i);
                    }

                    uint64_t hash = getHash<shifts>(permut, magic);
                    
                    // hash collision detected, break loop
                    if (attacks[i][hash] && attacks[i][hash] != attackBoard) {
                        isValidHash = false;
                        break;
                    }

                    attacks[i][hash] = attackBoard;

                    // move onto next permutation
                    permut = (permut - blockers) & blockers;
                } while (permut);

                // valid hash found, move on!
                if (isValidHash) {
                    magicArr[i].magic = magic;
                    break;
                }
            }
        }
    }

    template<uint8_t pieceType>
    void populateAttacks();

    template<>
    void populateAttacks<KING>() {
        for (int i = 0; i < MAX_SQUARE; i++) {
            int rank = getRank(i);
            int file = getFile(i);

            uint64_t attacks = 0;

            attacks |= getInBoundsMask(rank - 1, file - 1);
            attacks |= getInBoundsMask(rank - 1, file + 0);
            attacks |= getInBoundsMask(rank - 1, file + 1);
            attacks |= getInBoundsMask(rank + 0, file + 1);
            attacks |= getInBoundsMask(rank + 1, file + 1);
            attacks |= getInBoundsMask(rank + 1, file - 0);
            attacks |= getInBoundsMask(rank + 1, file - 1);
            attacks |= getInBoundsMask(rank - 0, file - 1);

            KING_ATTACKS[i] = attacks;
        }
    }

    template<>
    void populateAttacks<KNIGHT>() {
        for (int i = 0; i < MAX_SQUARE; i++) {
            int rank = getRank(i);
            int file = getFile(i);

            uint64_t attacks = 0;

            attacks |= getInBoundsMask(rank - 2, file - 1);
            attacks |= getInBoundsMask(rank - 2, file + 1);
            attacks |= getInBoundsMask(rank - 1, file + 2);
            attacks |= getInBoundsMask(rank + 1, file + 2);
            attacks |= getInBoundsMask(rank + 2, file + 1);
            attacks |= getInBoundsMask(rank + 2, file - 1);
            attacks |= getInBoundsMask(rank + 1, file - 2);
            attacks |= getInBoundsMask(rank - 1, file - 2);

            KNIGHT_ATTACKS[i] = attacks;
        }
    }

    void initMoveGen() {
        populateAttacks<KING>();
        populateAttacks<KNIGHT>();
        populateMagic<BISHOP>();
        populateMagic<ROOK>();
    }

    template<uint8_t pieceType>
    uint64_t getAttackBoard(uint8_t square, uint64_t occupancies) {
        static_assert(pieceType != PAWN);

        // non-sliding
        if constexpr (pieceType == KING) {
            return KING_ATTACKS[square];
        } else if constexpr (pieceType == KNIGHT) {
            return KNIGHT_ATTACKS[square];
        }

        uint64_t attackBoard = 0;

        // sliding
        if constexpr (pieceType == BISHOP || pieceType == QUEEN) {
            const Magic& magic = BISHOP_MAGIC[square];
            uint64_t relevantOccupancies = occupancies & magic.mask;
            uint16_t hash = getHash<BISHOP_SHIFTS>(relevantOccupancies, magic.magic);
            attackBoard |= BISHOP_ATTACKS[square][hash];
        } else if constexpr (pieceType == ROOK || pieceType == QUEEN) {
            const Magic& magic = ROOK_MAGIC[square];
            uint64_t relevantOccupancies = occupancies & magic.mask;
            uint16_t hash = getHash<ROOK_SHIFTS>(relevantOccupancies, magic.magic);
            attackBoard |= ROOK_ATTACKS[square][hash];
        }

        return attackBoard;
    }

    template<uint8_t pieceType, MoveType moveType>
    void genPieceMoves(const Board& board, MoveList& moveList) {
        static_assert(pieceType != PAWN);

        uint64_t movableSquares = 0;

        uint64_t pieces = board.bitboards[board.state.activeColor][pieceType];

        while (pieces != 0) {
            // pop LSB
            uint8_t initialLoc = countTrailingZeros(pieces);
            pieces = pieces & (pieces - 1);

            uint64_t attacks = getAttackBoard<pieceType>(initialLoc, board.getOccupiedSquares());

            if constexpr (moveType == MoveType::QUIET) {
                // you are not able to take any pieces
                attacks &= ~(board.getOccupiedSquares());
            } else if constexpr (moveType == MoveType::NOISY) {
                // you are only able to take opponent pieces
                attacks &= board.getOccupiedSquares(oppositeSide(board.state.activeColor));
            } else if constexpr (moveType == MoveType::ALL) {
                // you are not able to take your own pieces
                attacks &= ~(board.getOccupiedSquares(board.state.activeColor));
            }

            // king castling
            if constexpr (pieceType == KING && (moveType == MoveType::QUIET || moveType == MoveType::ALL)) {
                // squares required to be clear when castling
                static constexpr uint64_t CKINGSIDE_CASTLEABLE_SQUARES  = getMask(F1) | getMask(G1);
                static constexpr uint64_t CQUEENSIDE_CASTLEABLE_SQUARES = getMask(D1) | getMask(C1) | getMask(B1);

                // adjusted to the correct side (shift left 56 when side is black)
                uint64_t kingSideCastleableSquares = CKINGSIDE_CASTLEABLE_SQUARES << board.state.activeColor * 56;
                uint64_t queenSideCastleableSquares = CQUEENSIDE_CASTLEABLE_SQUARES << board.state.activeColor * 56;

                uint64_t occupiedSquares = board.getOccupiedSquares();

                uint64_t kingCastlingSquare = (board.state.activeColor == SIDE_WHITE) ? getMask(G1) : getMask(G8);
                uint64_t queenCastlingSquare = (board.state.activeColor == SIDE_WHITE) ? getMask(C1) : getMask(C8);

                bool canCastleKingSide = board.state.canCastle(board.state.activeColor, KING)
                                         && !(kingSideCastleableSquares & occupiedSquares);
                bool canCastleQueenSide = board.state.canCastle(board.state.activeColor, QUEEN)
                                          && !(queenSideCastleableSquares & occupiedSquares);

                attacks |= kingCastlingSquare * canCastleKingSide;
                attacks |= queenCastlingSquare * canCastleQueenSide;
            }

            // iterate through all end locations, add to move list
            while (attacks != 0) {
                // pop LSB
                uint8_t attackIndex = countTrailingZeros(attacks);
                attacks = attacks & (attacks - 1);

                moveList.push_back({ pieceType, initialLoc, attackIndex });
            }
        }
    }

    template<MoveType moveType>
    void genPawnMoves(const Board& board, MoveList& moveList) {
        uint8_t color = board.state.activeColor;

        // simple (hopefully compiler inlined) utility to extract moves from bb and add them to the move list
        auto addOffsetMoves = [&](uint64_t resultingBitboard, uint8_t amountShifted) {
            // iterate through all squares
            while (resultingBitboard != 0) {
                // pop LSB
                uint8_t index = countTrailingZeros(resultingBitboard);
                resultingBitboard = resultingBitboard & (resultingBitboard - 1);
                uint8_t startingIndex = (board.state.activeColor == SIDE_WHITE) ? index - amountShifted : index + amountShifted;
                moveList.push_back({ PAWN, startingIndex, index });
            }
        };
        
        auto addPromotionOffsetMoves = [&](uint64_t resultingBitboard, uint8_t amountShifted) {
            // iterate through all squares
            while (resultingBitboard != 0) {
                // pop LSB
                uint8_t index = countTrailingZeros(resultingBitboard);
                resultingBitboard = resultingBitboard & (resultingBitboard - 1);
                uint8_t startingIndex = (board.state.activeColor == SIDE_WHITE) ? index - amountShifted : index + amountShifted;
                moveList.push_back({ PAWN, startingIndex, index, QUEEN });
                moveList.push_back({ PAWN, startingIndex, index, KNIGHT });
                moveList.push_back({ PAWN, startingIndex, index, ROOK });
                moveList.push_back({ PAWN, startingIndex, index, BISHOP });
            }
        };

        auto shiftLeftBasedOnColor = [&](uint64_t val, uint8_t amount) {
            return color == SIDE_WHITE  ? val << amount : val >> amount;
        };

        uint64_t emptySquares = ~(board.getOccupiedSquares());
        uint64_t promoterMask = (color == SIDE_WHITE) ? BITBOARD_RANK_7 : BITBOARD_RANK_2;

        // quiet moves = pushing
        if constexpr (moveType == MoveType::QUIET || moveType == MoveType::ALL) {
            uint64_t doublePushRankMask = (color == SIDE_WHITE) ? BITBOARD_RANK_3 : BITBOARD_RANK_6;

            /* pushes */
            // push the pawns that are not about to promote, then remove all pushes with occupied squares in front
            uint64_t pushedPawns = shiftLeftBasedOnColor(board.bitboards[color][PAWN] & ~promoterMask, 8)
                                    & emptySquares;
            addOffsetMoves(pushedPawns, 8);

            /* double pushes */
            // push single-pushed pawns that are on 3rd or 6th rank a second time for double pushes
            uint64_t doublePushedPawns = shiftLeftBasedOnColor(pushedPawns & doublePushRankMask, 8) & emptySquares;
            addOffsetMoves(doublePushedPawns, 16);
        }

        // used for masking out the flank pawns for captures
        static constexpr uint64_t PAWN_LEFT_MASK[2]  = { BITBOARD_FILE_H, BITBOARD_FILE_A };
        static constexpr uint64_t PAWN_RIGHT_MASK[2] = { BITBOARD_FILE_A, BITBOARD_FILE_H };

        if constexpr (moveType == MoveType::NOISY || moveType == MoveType::ALL) {
            uint64_t promotedMask = (color == SIDE_WHITE) ? BITBOARD_RANK_8 : BITBOARD_RANK_1;

            /* captures */
            uint64_t oppSquares = board.getOccupiedSquares(oppositeSide(color));
            oppSquares |= IS_VALID_SQUARE(board.state.enpassantSquare) * getMask(board.state.enpassantSquare);
            uint64_t captureLPawns = shiftLeftBasedOnColor(board.bitboards[color][PAWN] & ~PAWN_RIGHT_MASK[color], 7) & oppSquares;
            uint64_t captureRPawns = shiftLeftBasedOnColor(board.bitboards[color][PAWN] & ~PAWN_LEFT_MASK[color], 9) & oppSquares;
            // normal captures
            addOffsetMoves(captureLPawns & ~promotedMask, 7);
            addOffsetMoves(captureRPawns & ~promotedMask, 9);
            // promotion captures
            addPromotionOffsetMoves(captureLPawns & promotedMask, 7);
            addPromotionOffsetMoves(captureRPawns & promotedMask, 9);

            /* promotion pushes */
            uint64_t promotedPawns = shiftLeftBasedOnColor(board.bitboards[color][PAWN] & promoterMask, 8) & emptySquares;
            addPromotionOffsetMoves(promotedPawns, 8);
        }
    }

    template<>
    void getMoves<MoveType::QUIET>(const Board& board, MoveList& moveList) {
        genPieceMoves<KING, MoveType::QUIET>(board, moveList);
        genPieceMoves<QUEEN, MoveType::QUIET>(board, moveList);
        genPieceMoves<BISHOP, MoveType::QUIET>(board, moveList);
        genPieceMoves<KNIGHT, MoveType::QUIET>(board, moveList);
        genPieceMoves<ROOK, MoveType::QUIET>(board, moveList);

        genPawnMoves<MoveType::NOISY>(board, moveList); // pawn moves are a bit quirky like that
    }

    template<>
    void getMoves<MoveType::NOISY>(const Board& board, MoveList& moveList) {
        genPieceMoves<KING, MoveType::NOISY>(board, moveList);
        genPieceMoves<QUEEN, MoveType::NOISY>(board, moveList);
        genPieceMoves<BISHOP, MoveType::NOISY>(board, moveList);
        genPieceMoves<KNIGHT, MoveType::NOISY>(board, moveList);
        genPieceMoves<ROOK, MoveType::NOISY>(board, moveList);

        genPawnMoves<MoveType::NOISY>(board, moveList); // pawn moves are a bit quirky like that
    }
    
    template<>
    void getMoves<MoveType::ALL>(const Board& board, MoveList& moveList) {
        genPieceMoves<KING, MoveType::ALL>(board, moveList);
        genPieceMoves<QUEEN, MoveType::ALL>(board, moveList);
        genPieceMoves<BISHOP, MoveType::ALL>(board, moveList);
        genPieceMoves<KNIGHT, MoveType::ALL>(board, moveList);
        genPieceMoves<ROOK, MoveType::ALL>(board, moveList);

        genPawnMoves<MoveType::ALL>(board, moveList); // pawn moves are a bit quirky like that
    }
} // namespace choco
