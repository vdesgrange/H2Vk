#pragma once

#include <vector>
#include "VkBootstrap.h"

struct DescriptorLayoutCache;

class DescriptorAllocator;

class DescriptorBuilder final {
public:
    static DescriptorBuilder begin(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator);

    DescriptorBuilder& bind_buffer(VkDescriptorBufferInfo& bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
    DescriptorBuilder& bind_image(VkDescriptorImageInfo& iInfo, VkDescriptorType type, VkDescriptorSet dst, VkShaderStageFlags stageFlags, uint32_t binding);
    DescriptorBuilder& bind_image(VkDescriptorImageInfo& iInfo, VkDescriptorType type, VkDescriptorSet dst, uint32_t binding);
    DescriptorBuilder& bind_none(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
    DescriptorBuilder& layout(VkDescriptorSetLayout& setLayout);

    bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout, std::vector<VkDescriptorPoolSize> sizes);
    void build(VkDescriptorSetLayout& setLayout);

private:
    std::vector<VkWriteDescriptorSet> _writes;
    std::vector<VkDescriptorSetLayoutBinding> _bindings;

    DescriptorLayoutCache* _cache;
    DescriptorAllocator* _alloc;
};