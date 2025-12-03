#include "zobrist.h"

namespace choco {
    void initZobrist() {
        std::mt19937_64 engine(600);
        for (size_t i = 0; i < 15; i++) {
            for (size_t j = 0; j < 64; j++) {
                ZOBRIST_HASHES[i][j] = engine();
            }
        }
    }
} // namespace choco
