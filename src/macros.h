#pragma once

#include <string>
#include <cstdint>

#define BOT_PERF_CTR

#ifdef BOT_PERF_CTR
namespace choco {
    extern uint64_t consideredMoves;
}
#endif

#define A1 (uint8_t)(0)
#define B1 (uint8_t)(1)
#define C1 (uint8_t)(2)
#define D1 (uint8_t)(3)
#define E1 (uint8_t)(4)
#define F1 (uint8_t)(5)
#define G1 (uint8_t)(6)
#define H1 (uint8_t)(7)

#define A2 (uint8_t)(8)
#define B2 (uint8_t)(9)
#define C2 (uint8_t)(10)
#define D2 (uint8_t)(11)
#define E2 (uint8_t)(12)
#define F2 (uint8_t)(13)
#define G2 (uint8_t)(14)
#define H2 (uint8_t)(15)

#define A3 (uint8_t)(16)
#define B3 (uint8_t)(17)
#define C3 (uint8_t)(18)
#define D3 (uint8_t)(19)
#define E3 (uint8_t)(20)
#define F3 (uint8_t)(21)
#define G3 (uint8_t)(22)
#define H3 (uint8_t)(23)

#define A4 (uint8_t)(24)
#define B4 (uint8_t)(25)
#define C4 (uint8_t)(26)
#define D4 (uint8_t)(27)
#define E4 (uint8_t)(28)
#define F4 (uint8_t)(29)
#define G4 (uint8_t)(30)
#define H4 (uint8_t)(31)

#define A5 (uint8_t)(32)
#define B5 (uint8_t)(33)
#define C5 (uint8_t)(34)
#define D5 (uint8_t)(35)
#define E5 (uint8_t)(36)
#define F5 (uint8_t)(37)
#define G5 (uint8_t)(38)
#define H5 (uint8_t)(39)

#define A6 (uint8_t)(40)
#define B6 (uint8_t)(41)
#define C6 (uint8_t)(42)
#define D6 (uint8_t)(43)
#define E6 (uint8_t)(44)
#define F6 (uint8_t)(45)
#define G6 (uint8_t)(46)
#define H6 (uint8_t)(47)

#define A7 (uint8_t)(48)
#define B7 (uint8_t)(49)
#define C7 (uint8_t)(50)
#define D7 (uint8_t)(51)
#define E7 (uint8_t)(52)
#define F7 (uint8_t)(53)
#define G7 (uint8_t)(54)
#define H7 (uint8_t)(55)

#define A8 (uint8_t)(56)
#define B8 (uint8_t)(57)
#define C8 (uint8_t)(58)
#define D8 (uint8_t)(59)
#define E8 (uint8_t)(60)
#define F8 (uint8_t)(61)
#define G8 (uint8_t)(62)
#define H8 (uint8_t)(63)

#define INVALID_SQUARE 64
#define IS_VALID_SQUARE(X) (X < 64)

static constexpr uint8_t SIDE_WHITE = 0;
static constexpr uint8_t SIDE_BLACK = 1;
static constexpr uint8_t oppositeSide(uint8_t side) { return !side; }
#define OPPOSITE_SIDE(SIDE) (!SIDE)

static constexpr uint8_t KING   = 0;
static constexpr uint8_t QUEEN  = 1;
static constexpr uint8_t BISHOP = 2;
static constexpr uint8_t KNIGHT = 3;
static constexpr uint8_t ROOK   = 4;
static constexpr uint8_t PAWN   = 5;
static constexpr uint8_t INVALID_PIECE = 6;
static constexpr bool isValidPiece(uint8_t piece) { return piece < 6; }
#define IS_VALID_PIECE(X) (X < 6)

static constexpr uint64_t BITBOARD_RANK_1 = 0xFFULL;
static constexpr uint64_t BITBOARD_RANK_2 = 0xFF00ULL;
static constexpr uint64_t BITBOARD_RANK_3 = 0xFF0000ULL;
static constexpr uint64_t BITBOARD_RANK_4 = 0xFF000000ULL;
static constexpr uint64_t BITBOARD_RANK_5 = 0xFF00000000ULL;
static constexpr uint64_t BITBOARD_RANK_6 = 0xFF0000000000ULL;
static constexpr uint64_t BITBOARD_RANK_7 = 0xFF000000000000ULL;
static constexpr uint64_t BITBOARD_RANK_8 = 0xFF00000000000000ULL;

static constexpr uint64_t BITBOARD_FILE_A = 0x0101010101010101ULL;
static constexpr uint64_t BITBOARD_FILE_B = 0x0202020202020202ULL;
static constexpr uint64_t BITBOARD_FILE_C = 0x0404040404040404ULL;
static constexpr uint64_t BITBOARD_FILE_D = 0x0808080808080808ULL;
static constexpr uint64_t BITBOARD_FILE_E = 0x1010101010101010ULL;
static constexpr uint64_t BITBOARD_FILE_F = 0x2020202020202020ULL;
static constexpr uint64_t BITBOARD_FILE_G = 0x4040404040404040ULL;
static constexpr uint64_t BITBOARD_FILE_H = 0x8080808080808080ULL;

static constexpr int MAX_MOVES = 218;

static const std::string STARTING_POS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
