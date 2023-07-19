/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <string>

#include "vk_command_buffer.h"
#include "core/utilities/vk_resources.h"

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

    bool load_image_from_file(const Device& device, const UploadContext& ctx, const char *file);
    bool load_image_from_buffer(const Device& device, const UploadContext& ctx, void *buffer, VkDeviceSize bufferSize, VkFormat format,
                                uint32_t texWidth, uint32_t texHeight); // , Image &outImage

    void updateDescriptor() {
        _descriptor.sampler = _sampler;
        _descriptor.imageView = _imageView;
        _descriptor.imageLayout = _imageLayout;
    }

    void destroy(const Device& device);
};
