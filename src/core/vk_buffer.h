#pragma once

#include "vk_mem_alloc.h"
#include "core/utilities/vk_types.h"

class Device;

struct AllocatedBuffer {
    VkBuffer _buffer;
    VmaAllocation _allocation;
};

class Buffer final {
public:
    static AllocatedBuffer create_buffer(const Device& device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
};