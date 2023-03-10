#pragma once

#include <unordered_map>
#include <string>

#include "core/utilities/vk_types.h"

class Device;
class VulkanEngine;

struct Texture {
    std::string _name;
    std::string _uri;
    VkImage _image;
    VkImageLayout _imageLayout; // duplicate with _descriptor
    VmaAllocation _allocation;
    VkImageView _imageView;  // duplicate
    VkSampler _sampler;  // duplicate
    VkDescriptorImageInfo _descriptor;
    uint32_t _width;
    uint32_t _height;

    bool load_image_from_file(VulkanEngine &engine, const char *file); // , AllocatedImage &outImage
    bool load_image_from_buffer(VulkanEngine &engine, void *buffer, VkDeviceSize bufferSize, VkFormat format,
                                uint32_t texWidth, uint32_t texHeight); // , Image &outImage

    void updateDescriptor() {
        _descriptor.sampler = _sampler;
        _descriptor.imageView = _imageView;
        _descriptor.imageLayout = _imageLayout;
    }

    void destroy(const Device& device);
};
