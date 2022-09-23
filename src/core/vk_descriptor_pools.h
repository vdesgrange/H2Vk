#pragma once

#include <vector>

#include "VkBootstrap.h"

class Device;

class DescriptorPools final {
public:
    VkDescriptorPool _descriptorPool;

    struct PoolSize {
        std::vector<VkDescriptorPoolSize> sizes = {
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
        };
    };

    DescriptorPools(const Device& device, std::vector<VkDescriptorPoolSize> sizes);
    ~DescriptorPools();

private:
    const class Device& _device;
};