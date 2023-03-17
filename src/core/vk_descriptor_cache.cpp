#include "vk_descriptor_cache.h"
#include "vk_device.h"

#include <algorithm>

DescriptorLayoutCache::~DescriptorLayoutCache() {
    for (auto setLayout : cache) {
        vkDestroyDescriptorSetLayout(_device._logicalDevice, setLayout.second, nullptr);
    }
}

VkDescriptorSetLayout DescriptorLayoutCache::createDescriptorLayout(VkDescriptorSetLayoutCreateInfo& info) {
    DescriptorLayoutInfo layoutInfo;
    bool sorted = true;
    int lastBinding = -1;

    layoutInfo._bindings.reserve(info.bindingCount);

    for (int i = 0; i < info.bindingCount; i++) {
        layoutInfo._bindings.push_back(info.pBindings[i]);
        if (info.pBindings[i].binding > lastBinding){
            lastBinding = info.pBindings[i].binding;
        } else{
            sorted = false;
        }
    }

    if (!sorted){
        std::sort(layoutInfo._bindings.begin(), layoutInfo._bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b ){
            return a.binding < b.binding;
        });
    }

    auto it = cache.find(layoutInfo);
    if (it != cache.end()){
        return (*it).second;
    } else {
        VkDescriptorSetLayout setLayout;
        vkCreateDescriptorSetLayout(_device._logicalDevice, &info, nullptr, &setLayout);
        cache[layoutInfo] = setLayout;
        return setLayout;
    }
}

bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(DescriptorLayoutCache::DescriptorLayoutInfo const& other) const {
    if (other._bindings.size() != _bindings.size()){
        return false;
    } else {
        for (int i = 0; i < _bindings.size(); i++) {
            if (other._bindings[i].binding != _bindings[i].binding) {
                return false;
            }
            if (other._bindings[i].descriptorType != _bindings[i].descriptorType) {
                return false;
            }
            if (other._bindings[i].descriptorCount != _bindings[i].descriptorCount) {
                return false;
            }
            if (other._bindings[i].stageFlags != _bindings[i].stageFlags) {
                return false;
            }
        }
        return true;
    }
}

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
