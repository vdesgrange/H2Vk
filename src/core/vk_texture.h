#pragma once

#include <unordered_map>
#include <string>

#include "vk_types.h"
#include "vk_engine.h"

class VulkanEngine;

namespace vkutil {
    bool load_image_from_file(VulkanEngine& engine, const char* file, AllocatedImage& outImage);
}

struct Texture final {
    AllocatedImage image;
    VkImageView imageView;
};

class TextureManager final {
public:
    class VulkanEngine& _engine;
    std::unordered_map<std::string, Texture> _loadedTextures;

    TextureManager(VulkanEngine& engine) : _engine(engine) {};
    ~TextureManager();

    void load_texture(const char* file, std::string name);

};

