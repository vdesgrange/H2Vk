/*
*  H2Vk - Buffer class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "core/utilities/vk_resources.h"

class Device;

/** @brief Allocated buffer */
struct AllocatedBuffer {
    /** @brief Represent linear arrays of data, used by binding them to a graphics or compute pipeline  */
    VkBuffer _buffer;
    /** @brief  Single memory allocation. Map/un-map to write data */
    VmaAllocation _allocation;
};

/**
 * Buffers are regions of memory used for storing arbitrary data that can be read by the graphics card.
 * @brief Provide functions to properly create a buffer
 */
class Buffer final {
public:
    static AllocatedBuffer create_buffer(const Device& device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
};