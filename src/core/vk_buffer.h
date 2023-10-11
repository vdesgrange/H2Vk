/*
*  H2Vk - Buffer class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "core/utilities/vk_resources.h"
#include <algorithm>

class Device;

/** @brief Allocated buffer */
struct AllocatedBuffer {
    /** @brief Principal memory allocator. One per device */
    VmaAllocator _allocator = nullptr;
    /** @brief  Single memory allocation. Map/un-map to write data */
    VmaAllocation _allocation = nullptr;
    /** @brief Represent linear arrays of data, used by binding them to a graphics or compute pipeline  */
    VkBuffer _buffer = VK_NULL_HANDLE;
    /** memory address */
    void* _data;

    ~AllocatedBuffer() {
        // destroy();
    }

    VkResult map();
    void unmap();
    void copyFrom(void* source, size_t size);
    void destroy();
};

/**
 * Buffers are regions of memory used for storing arbitrary data that can be read by the graphics card.
 * @brief Provide functions to properly create a buffer
 * todo - add mapping/unmapping, destruction, etc.
 */
class Buffer final {
public:
    static AllocatedBuffer create_buffer(const Device& device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    static void create_buffer(const Device& device, AllocatedBuffer* buffer, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
};