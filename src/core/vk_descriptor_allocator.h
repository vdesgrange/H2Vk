/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <vector>

#include "core/utilities/vk_resources.h"

class Device;

/**
 * DescriptorAllocator
 * This might not be the most optimal solution.
 * - Maximal number of sets per pool is abstract. Pool size fixed by descriptor layout.
 * - Descriptor pools are created when pool size does not suits the best (which depends on descriptor layout)
 * while a single descriptor pool with large range of pool sizes might work instead.
 * todo - Look for a proper solution?
 */
class DescriptorAllocator final {
public:
    /** @brief vulkan wrapper device */
    const class Device& _device;
    /** @brief Number of sets per pools (kind of abstract so far) */
    const unsigned int MAX_SETS = 10;

    DescriptorAllocator(const Device &device) : _device(device) {};
    ~DescriptorAllocator();

    bool allocate(VkDescriptorSet* descriptor, VkDescriptorSetLayout* setLayout, std::vector<VkDescriptorPoolSize> sizes);

    void resetPools();
    VkDescriptorPool getPool(std::vector<VkDescriptorPoolSize> sizes, VkDescriptorPoolCreateFlags flags, uint32_t count);
    VkDescriptorPool createPool(std::vector<VkDescriptorPoolSize> sizes, VkDescriptorPoolCreateFlags flags, uint32_t count);

private:
    /** @brief pool set as main allocation pool */
    VkDescriptorPool _currentPool{VK_NULL_HANDLE};
    /** @brief collection of used descriptor pools */
    std::vector<VkDescriptorPool> usedPools;
    /** @brief collection of free descriptor pools. Can be used for descriptor set allocation. */
    std::vector<VkDescriptorPool> freePools;
};