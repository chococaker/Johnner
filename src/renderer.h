#pragma once

#include <raylib.h>
#include "bitboard.h"
#include "texture_manager.h"

namespace choco {
    class Renderer {
    public:
        Renderer(TextureMgr& textureMgr);
        
        void init() const;
        void render(const Board& bitboard) const;
    private:
        TextureMgr& textureMgr;
    };
} // namespace choco
