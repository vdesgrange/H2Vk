#pragma once

#include <unordered_map>
#include <string>

#include "vk_types.h"

class VulkanEngine;
// struct Image;

// namespace vkutil {
    // bool load_image_from_file(VulkanEngine &engine, const char *file, AllocatedImage &outImage);
//    bool load_image_from_buffer(VulkanEngine &engine, void *buffer, VkDeviceSize bufferSize, VkFormat format,
//                                        uint32_t texWidth, uint32_t texHeight, Image &outImage);
//}

struct Texture {
//    int32_t index;
//    AllocatedImage image;
//    VkImageView imageView;

    VkImage _image;
    VkImageLayout _imageLayout; // duplicate with _descriptor
    VmaAllocation _allocation;
    VkImageView _imageView;  // duplicate
    VkSampler _sampler;  // duplicate
    VkDescriptorImageInfo _descriptor;

    bool load_image_from_file(VulkanEngine &engine, const char *file); // , AllocatedImage &outImage
    bool load_image_from_buffer(VulkanEngine &engine, void *buffer, VkDeviceSize bufferSize, VkFormat format,
                                uint32_t texWidth, uint32_t texHeight); // , Image &outImage

    void updateDescriptor() {
        _descriptor.sampler = _sampler;
        _descriptor.imageView = _imageView;
        _descriptor.imageLayout = _imageLayout;
    }
};

class TextureManager final {
public:
    class VulkanEngine& _engine;
    std::unordered_map<std::string, Texture> _loadedTextures;

    TextureManager(VulkanEngine& engine) : _engine(engine) {};
    ~TextureManager();

    void load_texture(const char* file, std::string name);
};


class SamplerManager final {
public:
    class VulkanEngine& _engine;
    std::unordered_map<std::string, VkSampler> _loadedSampler;

    SamplerManager(VulkanEngine& engine) : _engine(engine) {};
    ~SamplerManager();

};