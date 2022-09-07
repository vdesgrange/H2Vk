#pragma once

#include "vk_mem_alloc.h"
#include "vk_types.h"

class Device;

class Buffer final {
public:
    static AllocatedBuffer create_buffer(const Device& device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
};