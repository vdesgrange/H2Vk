/*
*  H2Vk - Texture structure
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <string>

#include "vk_command_buffer.h"
#include "core/utilities/vk_resources.h"

struct Sampler {
    VkFilter magFilter;
    VkFilter minFilter;
    VkSamplerAddressMode addressModeU;
    VkSamplerAddressMode addressModeV;
    VkSamplerAddressMode addressModeW;
};

struct Texture {
    /** @brief Texture appellation */
    std::string _name;
    /** @brief Texture URI */
    std::string _uri;
    /** @brief Image object. Handle actual texture data. To be bind to graphics or compute pipeline */
    VkImage _image = VK_NULL_HANDLE;
    /** @brief Layout of image and its sub-ressources. In-memory image pixels organization. */
    VkImageLayout _imageLayout; // duplicate with _descriptor
    /** @brief Single memory allocation. */
    VmaAllocation _allocation;
    /** @brief Image wrapper. Hold information on access image data. Handle metadata, mipmap, etc. */
    VkImageView _imageView =  VK_NULL_HANDLE;  // duplicate
    /** @brief How image is sampled by shader (filter, blending...) */
    VkSampler _sampler = VK_NULL_HANDLE;  // duplicate

    /** @brief Descriptor image information. Duplicates with image layout, view, sampler */
    VkDescriptorImageInfo _descriptor;

    /** @brief Texture width */
    uint32_t _width;
    /** @brief Texture height */
    uint32_t _height;

    bool load_image_from_file(const Device& device, const UploadContext& ctx, const char *file);
    bool load_image_from_buffer(const Device& device, const UploadContext& ctx, void *buffer, VkDeviceSize bufferSize, VkFormat format,
                                uint32_t texWidth, uint32_t texHeight);
    bool load_image_from_buffer(const Device& device, const UploadContext& ctx, void *buffer, VkDeviceSize bufferSize, Sampler& sampler, VkFormat format,
                                uint32_t texWidth, uint32_t texHeight);
    /**
     * Update image descriptor texture attributes (sampler, imageView, imageLayout)
     */
    void updateDescriptor() {
        _descriptor.sampler = _sampler;
        _descriptor.imageView = _imageView;
        _descriptor.imageLayout = _imageLayout;
    }

    void destroy(const Device& device);
};
