#pragma once

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>

#include "macros.h"

namespace choco {
    inline uint8_t countTrailingZeros(uint64_t n) {
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_ctzll(n);
#elif defined(_MSC_VER)
        return __tzcnt_u64(n);
#else
        unsigned int count = 0;
        while ((n & 1) == 0 && count < 64) {
            n >>= 1;
            count++;
        }
        return count;
#endif
    }
    inline uint8_t countOnes(uint64_t n) {
#if defined(__GNUC__) || defined(__clang__)
        return static_cast<uint8_t>(__builtin_popcountll(n));
#elif defined(_MSC_VER)
        return static_cast<uint8_t>(__popcnt64(n));
#else 
        uint8_t count = 0;
        while (n > 0) {
            n &= (n - 1);
            count++;
        }
        return count;
#endif
    }

    inline uint64_t getFileMask(int file) {
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

        return 0ULL;
    }

    inline uint64_t getRankMask(int rank) {
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

        return 0ULL;
    }

    constexpr inline uint8_t getIndex(int rank, int file) {
        return (rank * 8) + file;
    }

    constexpr inline uint8_t getRank(uint8_t index) {
        return index / 8;
    }

    constexpr inline uint8_t getFile(uint8_t index) {
        return index % 8;
    }

    constexpr inline uint64_t getMask(uint8_t index) {
        return 1ULL << index;
    }

    constexpr inline uint64_t getMask(uint8_t rank, uint8_t file) {
        return getMask(getIndex(rank, file));
    }

    inline void iterateIndices(uint64_t bitboard, auto&& func) {
        while (bitboard != 0) {
            uint8_t index = countTrailingZeros(bitboard);
            bitboard = bitboard & (bitboard - 1);
            func(index);
        }
    }

    constexpr inline uint8_t getPieceOnSquare(const uint64_t bitboards[6], uint8_t square) {
        uint64_t mask = getMask(square);
        for (uint8_t i = 0; i < 6; i++) {
            if (bitboards[i] & mask) return i;
        }

        return INVALID_PIECE;
    }
    constexpr inline uint8_t getPieceOnSquare(const uint64_t bitboards[2][6], uint8_t square) {
        uint8_t piece = getPieceOnSquare(bitboards[SIDE_WHITE], square);
        return (piece != INVALID_PIECE) ? piece : getPieceOnSquare(bitboards[SIDE_BLACK], square);
    }

    template<typename T>
    constexpr inline T unsignedDist(T a, T b) {
         return a > b ? a - b : b - a;
    }
} // namespace choco
