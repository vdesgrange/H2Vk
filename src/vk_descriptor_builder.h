#pragma once

#include <vector>
#include "VkBootstrap.h"

struct DescriptorLayoutCache;
class DescriptorAllocator;

class DescriptorBuilder final {
public:
    static DescriptorBuilder begin(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator);

    DescriptorBuilder& bind_buffer(uint32_t binding, VkDescriptorBufferInfo& bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
    DescriptorBuilder& bind_image(uint32_t binding, VkDescriptorImageInfo& imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

    bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout, std::vector<VkDescriptorPoolSize> sizes);
    bool build(VkDescriptorSet& set);
private:

    std::vector<VkWriteDescriptorSet> _writes;
    std::vector<VkDescriptorSetLayoutBinding> _bindings;

    DescriptorLayoutCache* _cache;
    DescriptorAllocator* _alloc;
};