#pragma once

#include <vector>

#include "board.h"

namespace choco {
    template<typename Data>
    class Node {
        Data data;
        std::vector<Node*> children;
    };
} // namespace choco
