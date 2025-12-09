#include "types.h"

#include <utility>

namespace choco {
    bool Move::operator==(const Move& other) const {
        return pieceType == other.pieceType
                && from == other.from
                && to == other.to
                && promotionType == other.promotionType;
    }

    MoveList::MoveList() : i(0) { }

    Move& MoveList::operator[](size_t n) {
        return moves[n];
    }
    const Move& MoveList::operator[](size_t n) const {
        return moves[n];
    }
    
    uint8_t MoveList::size() const {
        return i;
    }

    Move MoveList::pop() {
        Move& m = moves[i--];
        return m;
    }

    void MoveList::push_back(Move&& move) {
        moves[i++] = std::move(move);
    }

    void MoveList::push_back(const Move& move) {
        moves[i++] = move;
    }

    void MoveList::swap(uint8_t n1, uint8_t n2) {
        std::swap(moves[n1], moves[n2]);
    }

    MoveList::Iterator MoveList::begin() {
        return Iterator(moves); // Points to the first element
    }

    MoveList::Iterator MoveList::end() {
        return Iterator(moves + i); // Points to one past the last valid element
    }
} // namespace choco
