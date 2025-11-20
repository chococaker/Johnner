#include "types.h"

namespace choco {
    MoveList::MoveList() : i(0) { }

    Move& MoveList::operator[](size_t n) {
        return moves[i];
    }
    const Move& MoveList::operator[](size_t n) const {
        return moves[i];
    }
    
    uint8_t MoveList::size() const {
        return i;
    }

    Move MoveList::pop() {
        Move& m = moves[i];
        i--;
        return m;
    }

    void MoveList::push_back(const Move& move) {
        moves[i] = move;
        i++;
    }

    void MoveList::swap(uint8_t n1, uint8_t n2) {
        Move temp = (*this)[n2];
        (*this)[n2] = (*this)[n1];
        (*this)[n1] = temp;
    }

    MoveList::Iterator MoveList::begin() {
        return Iterator(moves); // Points to the first element
    }

    MoveList::Iterator MoveList::end() {
        return Iterator(moves + i); // Points to one past the last valid element
    }
} // namespace choco
