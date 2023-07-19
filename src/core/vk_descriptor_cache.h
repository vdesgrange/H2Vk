/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <vector>
#include <unordered_map>

#include "core/utilities/vk_resources.h"

class Device;

class DescriptorLayoutCache final {
public:
    struct DescriptorLayoutInfo {
        std::vector<VkDescriptorSetLayoutBinding> _bindings;
        bool operator==(const DescriptorLayoutInfo& other) const;
        size_t hash() const;
    };

    DescriptorLayoutCache(const Device &device) : _device(device) {};
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