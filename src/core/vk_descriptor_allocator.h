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
    const class Device& _device;
    const unsigned int MAX_SETS = 10; // Number of sets per pools (kind of abstract so far)

    DescriptorAllocator(const Device &device) : _device(device) {};
    ~DescriptorAllocator();

    bool allocate(VkDescriptorSet* descriptor, VkDescriptorSetLayout* setLayout, std::vector<VkDescriptorPoolSize> sizes);

    void resetPools();
    VkDescriptorPool getPool(std::vector<VkDescriptorPoolSize> sizes, VkDescriptorPoolCreateFlags flags, uint32_t count);
    VkDescriptorPool createPool(std::vector<VkDescriptorPoolSize> sizes, VkDescriptorPoolCreateFlags flags, uint32_t count);

private:
    VkDescriptorPool _currentPool{VK_NULL_HANDLE};
    std::vector<VkDescriptorPool> usedPools;
    std::vector<VkDescriptorPool> freePools;
};