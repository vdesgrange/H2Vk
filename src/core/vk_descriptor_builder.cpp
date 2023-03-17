#include "vk_descriptor_builder.h"
#include "vk_descriptor_allocator.h"
#include "vk_descriptor_cache.h"
#include "vk_device.h"
#include "core/utilities/vk_initializers.h"

DescriptorBuilder DescriptorBuilder::begin(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator) {
    DescriptorBuilder builder;
    builder._cache = &layoutCache;
    builder._alloc = &allocator;
    return builder;
}

DescriptorBuilder& DescriptorBuilder::bind_buffer(VkDescriptorBufferInfo& bInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {
    VkDescriptorSetLayoutBinding bind = vkinit::descriptor_set_layout_binding(type, stageFlags, binding);
    _bindings.push_back(bind);

    VkWriteDescriptorSet write = vkinit::write_descriptor_set(type, nullptr, &bInfo, binding);
    _writes.push_back(write);

    return *this;
}

DescriptorBuilder& DescriptorBuilder::bind_image(VkDescriptorImageInfo& iInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {
    VkDescriptorSetLayoutBinding bind = vkinit::descriptor_set_layout_binding(type, stageFlags, binding);
    _bindings.push_back(bind);

    VkWriteDescriptorSet write = vkinit::write_descriptor_image(type, nullptr, &iInfo, binding); // dstSet handled dans DescriptorBuilder::build
    _writes.push_back(write);

    return *this;
}

DescriptorBuilder& DescriptorBuilder::bind_none(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {
    VkDescriptorSetLayoutBinding bind = vkinit::descriptor_set_layout_binding(type, stageFlags, binding);
    _bindings.push_back(bind);

    return *this;
}

DescriptorBuilder& DescriptorBuilder::layout(VkDescriptorSetLayout& setLayout) {
    VkDescriptorSetLayoutCreateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.pNext = nullptr;
    setInfo.flags = 0;
    setInfo.bindingCount = static_cast<uint32_t>(_bindings.size());
    setInfo.pBindings = _bindings.data();

    setLayout = _cache->createDescriptorLayout(setInfo);

    return *this;
}

bool DescriptorBuilder::build(VkDescriptorSet& set, VkDescriptorSetLayout& setLayout, std::vector<VkDescriptorPoolSize> sizes) {
    bool success = _alloc->allocate(&set, &setLayout, sizes);
    if (!success) {
        return false;
    };

    for (VkWriteDescriptorSet& w : _writes) {
        w.dstSet = set;
    }

    vkUpdateDescriptorSets(_alloc->_device._logicalDevice, _writes.size(), _writes.data(), 0, nullptr);

    return true;
}