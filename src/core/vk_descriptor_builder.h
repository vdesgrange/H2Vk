/*
*  H2Vk - DescriptorBuilder class
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

/**
 * DescriptorBuilder is a set of tools to bind different resources (ie. buffer, images), generate a descriptor layout
 * and create descriptor set which will be used in different shader stages.
 * Number of descriptor sets which can be bound is limited (ie. 8 for Mac)
 * @brief descriptor set builder
 * @note each method return a reference to the instance in a functional fashion.
 */
class DescriptorBuilder final {
public:
    static DescriptorBuilder begin(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator);

    DescriptorBuilder& bind_buffer(VkDescriptorBufferInfo& bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
    DescriptorBuilder& bind_image(VkDescriptorImageInfo& iInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
    DescriptorBuilder& bind_none(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
    DescriptorBuilder& layout(VkDescriptorSetLayout& setLayout);

    bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout, std::vector<VkDescriptorPoolSize> sizes);

private:
    /** @brief collection of write operation descriptions (ie. destination set, binding, pointer to image, buffer...)*/
    std::vector<VkWriteDescriptorSet> _writes;
    /** @brief collection of bindings used to create a descriptor set */
    std::vector<VkDescriptorSetLayoutBinding> _bindings;

    /** @brief descriptor set layout wrapper + caching */
    DescriptorLayoutCache* _cache;
    DescriptorAllocator* _alloc;
};