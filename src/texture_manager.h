#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <iostream>

namespace choco {
    class TextureMgr {
    public:
        void loadAsset(const std::string& id, const std::string& path) {
            if (textures.count(id)) {
                throw std::invalid_argument("Already loaded ID " + id);
            }
            std::cout << "Loading " << id << std::endl;
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

