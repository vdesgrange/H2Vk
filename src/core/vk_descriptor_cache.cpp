/*
*  H2Vk - DescriptorLayoutCache class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_descriptor_cache.h"

#include <algorithm>

/**
 * Delete every descriptor set layout.
 * @brief default destructor
 */
DescriptorLayoutCache::~DescriptorLayoutCache() {
    for (auto setLayout : cache) {
        vkDestroyDescriptorSetLayout(_device._logicalDevice, setLayout.second, nullptr);
    }
}

/**
 * Create descriptor set layout from list of bindings provided.
 * Each individual descriptor binding is specified by a descriptor type, number of
 * descriptors in the binding, a set of shader stages that can access the binding.
 * @brief create & cache or return cached descriptor set layout
 * @param info parameters of a newly created descriptor set layout. Contains list of bindings.
 * @return descriptor set layout
 */
VkDescriptorSetLayout DescriptorLayoutCache::createDescriptorLayout(VkDescriptorSetLayoutCreateInfo& info) {
    DescriptorLayoutInfo layoutInfo;
    bool sorted = true;
    int lastBinding = -1;

    layoutInfo._bindings.reserve(info.bindingCount);

    for (int i = 0; i < info.bindingCount; i++) {
        layoutInfo._bindings.push_back(info.pBindings[i]); // add layout binding description
        if (info.pBindings[i].binding > lastBinding){
            lastBinding = info.pBindings[i].binding;
        } else{
            sorted = false;
        }
    }

    if (!sorted){ // sort to get proper operator comparison ==
        std::sort(layoutInfo._bindings.begin(), layoutInfo._bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b ){
            return a.binding < b.binding;
        });
    }

    auto it = cache.find(layoutInfo); // use overload operator () with hash
    if (it != cache.end()){
        return (*it).second;
    } else {
        VkDescriptorSetLayout setLayout;
        vkCreateDescriptorSetLayout(_device._logicalDevice, &info, nullptr, &setLayout);
        cache[layoutInfo] = setLayout;
        return setLayout;
    }
}

/**
 * Determine descriptor set layout equality based on number of bindings, binding index, type of resource,
 * number of descriptors and pipeline shader stages with access to this binding.
 * @brief overload DescriptorLayoutInfo operator ==
 * @param other
 * @return
 */
bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(DescriptorLayoutCache::DescriptorLayoutInfo const& other) const {
    if (other._bindings.size() != _bindings.size()){
        return false;
    } else {
        for (int i = 0; i < _bindings.size(); i++) {
            if (other._bindings[i].binding != _bindings[i].binding) { // index
                return false;
            }
            if (other._bindings[i].descriptorType != _bindings[i].descriptorType) { // type
                return false;
            }
            if (other._bindings[i].descriptorCount != _bindings[i].descriptorCount) { // number of descriptor
                return false;
            }
            if (other._bindings[i].stageFlags != _bindings[i].stageFlags) { // shader stage with access
                return false;
            }
        }
        return true;
    }
}

/**
 * Hash made from binding, descriptor type, count and stage flags.
 * @brief Descriptor layout info hash function
 * @return hash
 */
size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const{
    using std::size_t;
    using std::hash;
    size_t result = hash<size_t>()(_bindings.size());

    for (const VkDescriptorSetLayoutBinding& b : _bindings) {
        size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;
        result ^= hash<size_t>()(binding_hash);
    }

    return result;
}
