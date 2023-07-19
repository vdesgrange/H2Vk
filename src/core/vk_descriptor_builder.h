/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <vector>
#include "core/utilities/vk_resources.h"

struct DescriptorLayoutCache;

class DescriptorAllocator;

class DescriptorBuilder final {
public:
    static DescriptorBuilder begin(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator);

    DescriptorBuilder& bind_buffer(VkDescriptorBufferInfo& bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
    DescriptorBuilder& bind_image(VkDescriptorImageInfo& iInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
    DescriptorBuilder& bind_none(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
    DescriptorBuilder& layout(VkDescriptorSetLayout& setLayout);

    bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout, std::vector<VkDescriptorPoolSize> sizes);

private:
    std::vector<VkWriteDescriptorSet> _writes;
    std::vector<VkDescriptorSetLayoutBinding> _bindings;

    DescriptorLayoutCache* _cache;
    DescriptorAllocator* _alloc;
};