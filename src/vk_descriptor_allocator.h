#pragma once

#include <vector>

#include "VkBootstrap.h"

class Device;

class DescriptorAllocator final {
public:
    DescriptorAllocator(const Device& device);
    ~DescriptorAllocator();
    VkDescriptorPool _currentPool;
    const class Device& _device;

    bool allocate(VkDescriptorSet& set, VkDescriptorSetLayout& setLayout, std::vector<VkDescriptorPoolSize> sizes);
    void resetPools();
    VkDescriptorPool getPool(std::vector<VkDescriptorPoolSize> sizes, VkDescriptorPoolCreateFlags flags, uint32_t count);
    VkDescriptorPool createPool(std::vector<VkDescriptorPoolSize> sizes, VkDescriptorPoolCreateFlags flags, uint32_t count);

private:
    std::vector<VkDescriptorPool> usedPools;
    std::vector<VkDescriptorPool> freePools;
};