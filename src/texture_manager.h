#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>
#include <stdexcept>

namespace choco {
    class TextureMgr {
    public:
        void loadAsset(std::string id, const std::string& path) {
            if (textures.count(id)) {
                throw std::invalid_argument("Already loaded ID " + id);
            }
            Image img = LoadImage(path.c_str());
            Texture2D asset = LoadTextureFromImage(img);
            UnloadImage(img);

            textures[id] = asset;
        }

        Texture2D getAsset(const std::string& id) {
            if (!textures.count(id)) {
                throw std::invalid_argument("Unknown texture ID " + id);
            }

            return textures[id];
        }
    
    private:
        std::unordered_map<std::string, Texture2D> textures;
    };
}

