#pragma once

#include <vector>
#include <unordered_map>

#include "VkBootstrap.h"

class Device;

class DescriptorLayoutCache final {
public:
    struct DescriptorLayoutInfo {
        std::vector<VkDescriptorSetLayoutBinding> _bindings;
        bool operator==(const DescriptorLayoutInfo& other) const;
        size_t hash() const;
    };

    std::vector<VkDescriptorSetLayout> _setLayouts;
    DescriptorLayoutInfo _layoutInfo;

    DescriptorLayoutCache(const Device& device);
    ~DescriptorLayoutCache();
    VkDescriptorSetLayout createDescriptorLayout(VkDescriptorSetLayoutCreateInfo& info);

private:
    struct DescriptorLayoutHash {
        std::size_t operator()(const DescriptorLayoutInfo& k) const{
            return k.hash();
        }
    };

    const class Device& _device;
    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> cache;
};