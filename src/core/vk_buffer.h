#pragma once

#include "core/utilities/vk_resources.h"

class Device;

struct AllocatedBuffer {
    VkBuffer _buffer;
    VmaAllocation _allocation;
};

class Buffer final {
public:
    static AllocatedBuffer create_buffer(const Device& device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
};