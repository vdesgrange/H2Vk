/*
*  H2Vk - DescriptorBuilder class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_descriptor_builder.h"
#include "vk_descriptor_allocator.h"
#include "vk_descriptor_cache.h"
#include "core/utilities/vk_initializers.h"

/**
 * Initialize DescriptorBuilder with caching and allocation systems.
 * @brief constructor
 * @param layoutCache caching system for this builder
 * @param allocator allocator system for this builder
 * @return descriptor builder
 */
DescriptorBuilder DescriptorBuilder::begin(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator) {
    DescriptorBuilder builder;
    builder._cache = &layoutCache;
    builder._alloc = &allocator;
    return builder;
}

/**
 * To specify buffer binding
 * @param bInfo descriptor buffer information. buffer resource, memory offset (relative to descriptor starting offset) and size.
 * @param type descriptor type (ie. uniform buffer, storage buffer)
 * @param stageFlags bitmask to specify shader stages (ie. vertex, fragment, tesselation, compute, etc.)
 * @param binding index of binding
 * @return reference to instance
 */
DescriptorBuilder& DescriptorBuilder::bind_buffer(VkDescriptorBufferInfo& bInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {
    VkDescriptorSetLayoutBinding bind = vkinit::descriptor_set_layout_binding(type, stageFlags, binding);
    _bindings.push_back(bind);

    VkWriteDescriptorSet write = vkinit::write_descriptor_set(type, nullptr, &bInfo, binding);
    _writes.push_back(write);

    return *this;
}

/**
 * To specify image binding
 * @param iInfo descriptor image info. sampler, image view and layout.
 * @param type descriptor type (ie. combined image sampler)
 * @param stageFlags bitmask to specify shader stages (ie. vertex, fragment, tesselation, compute, etc.)
 * @param binding index of binding
 * @return reference to DescriptorBuilder instance
 */
DescriptorBuilder& DescriptorBuilder::bind_image(VkDescriptorImageInfo& iInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {
    VkDescriptorSetLayoutBinding bind = vkinit::descriptor_set_layout_binding(type, stageFlags, binding);
    _bindings.push_back(bind);

    VkWriteDescriptorSet write = vkinit::write_descriptor_image(type, nullptr, &iInfo, binding); // dstSet handled dans DescriptorBuilder::build
    _writes.push_back(write);

    return *this;
}

/**
 * Used when nothing to write into descriptor set but need to reserved space for binding (ie. future image).
 * @param type descriptor type
 * @param stageFlags bitmask to specify shader stages (ie. vertex, fragment, tesselation, compute, etc.)
 * @param binding  index of binding
 * @return reference to DescriptorBuilder instance
 */
DescriptorBuilder& DescriptorBuilder::bind_none(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {
    VkDescriptorSetLayoutBinding bind = vkinit::descriptor_set_layout_binding(type, stageFlags, binding);
    _bindings.push_back(bind);

    return *this;
}

/**
 * create + cache or get cached layout (using class attributes), then update descriptor set layout by reference
 * @param setLayout descriptor set layout to update by reference
 * @return reference to DescriptorBuilder instance
 */
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

/**
 * Update content of a referenced descriptor set object based on previously set layout and binding.
 * @brief update descriptor set
 * @param set descriptor set, set of pointers into resources (ie. buffer, images).
 * @param setLayout layout of the descriptor set, structure of the binding resources
 * @param sizes
 * @return true if descriptor set successfully updated
 */
bool DescriptorBuilder::build(VkDescriptorSet& set, VkDescriptorSetLayout& setLayout, std::vector<VkDescriptorPoolSize> sizes) {
    bool success = _alloc->allocate(&set, &setLayout, sizes);
    if (!success) {
        return false;
    };

    for (VkWriteDescriptorSet& w : _writes) { //
        w.dstSet = set; // update writing operation's destination set to the referenced descriptor set.
    }
    // update content of the descriptor set object, make descriptor set point to resources.
    // possible before bounded for first time (or command buffer submitted).
    vkUpdateDescriptorSets(_alloc->_device._logicalDevice, _writes.size(), _writes.data(), 0, nullptr);

    return true;
}