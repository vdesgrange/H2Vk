#include "vk_buffer.h"
#include "vk_helpers.h"
#include "vk_device.h"

/**
 * AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
 * Buffers are regions of memory used for storing arbitrary data that can be read by the graphics card.
 * @param allocSize
 * @param usage
 * @param memoryUsage
 * @return
 */
AllocatedBuffer Buffer::create_buffer(const Device& device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = memoryUsage;

    AllocatedBuffer newBuffer;
    VK_CHECK(vmaCreateBuffer(device._allocator,
                             &bufferInfo,
                             &vmaallocInfo,
                             &newBuffer._buffer,
                             &newBuffer._allocation,
                             nullptr));

    return newBuffer;
}