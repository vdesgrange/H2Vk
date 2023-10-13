/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_buffer.h"

/**
 * Create a buffer object (VkBuffer), get memory requirements,
 * allocate memory (VkDeviceMemory) using allocator then bind buffer with memory.
 * @brief Allocate buffer properly
 * @param device reference to vulkan device wrapper
 * @param allocSize size in bytes of the buffer to be created
 * @param usage  bitmask specifying allowed usages of the buffer
 * @param memoryUsage memory requirements
 * @return
 */
// AllocatedBuffer Buffer::create_buffer(const Device& device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
//     // Parameters of a newly created buffer object
//     VkBufferCreateInfo bufferInfo = {};
//     bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//     bufferInfo.pNext = nullptr;
//     bufferInfo.size = allocSize;
//     bufferInfo.usage = usage;
//     bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

//     // Parameters of new VmaAllocation.
//     VmaAllocationCreateInfo vmaallocInfo = {};
//     vmaallocInfo.usage = memoryUsage;
//     vmaallocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

//     // Creates a new VkBuffer
//     AllocatedBuffer newBuffer;
//     newBuffer._allocator = device._allocator;
//     VK_CHECK(vmaCreateBuffer(device._allocator,
//                              &bufferInfo,
//                              &vmaallocInfo,
//                              &newBuffer._buffer,
//                              &newBuffer._allocation,
//                              nullptr));

//     return newBuffer;
// }

void Buffer::create_buffer(const Device& device, AllocatedBuffer* buffer, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
    // Parameters of a newly created buffer object
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Parameters of new VmaAllocation.
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = memoryUsage;
    vmaallocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    // Creates a new VkBuffer
    buffer->_allocator = device._allocator;
    VK_CHECK(vmaCreateBuffer(device._allocator,
                             &bufferInfo,
                             &vmaallocInfo,
                             &buffer->_buffer,
                             &buffer->_allocation,
                             nullptr));

}

VkResult AllocatedBuffer::map() {
    return vmaMapMemory(_allocator, _allocation, &_data);
}

void AllocatedBuffer::unmap() {
    if (_data) {
        vmaUnmapMemory(_allocator, _allocation);
    }
}

void AllocatedBuffer::copyFrom(void* source, size_t size) {
    assert(_data);
    memcpy(_data, source, size);
}

void AllocatedBuffer::destroy() {
    if (_allocator) {
        vmaDestroyBuffer(_allocator, _buffer, _allocation);
    }
}