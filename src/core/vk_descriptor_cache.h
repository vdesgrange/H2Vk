/*
*  H2Vk - DescriptorLayoutCache class
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

/**
 *
 * Descriptor set layout object is defined by an array of zero or more descriptor bindings.
 * Layout is the structure of the descriptor (ie. 2 buffers and 1 image)
 *
 * @brief Descriptor layout wrapper + caching
 */
class DescriptorLayoutCache final {
public:
    /** @brief collection of binding with hash function */
    struct DescriptorLayoutInfo {
        /** @brief collection of descriptor set layout bindings */
        std::vector<VkDescriptorSetLayoutBinding> _bindings;
        bool operator==(const DescriptorLayoutInfo& other) const;
        size_t hash() const;
    };

    DescriptorLayoutCache(const Device &device) : _device(device) {};
    ~DescriptorLayoutCache();
    VkDescriptorSetLayout createDescriptorLayout(VkDescriptorSetLayoutCreateInfo& info);

private:
    /** @brief hash structure used for descriptor layout caching */
    struct DescriptorLayoutHash { // Defined as separate class to avoid specialization in std namespace
        std::size_t operator()(const DescriptorLayoutInfo& k) const{ // (find).operator()(x...)
            return k.hash();
        }
    };

    const class Device& _device;
    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> cache;
};