#include "VkBootstrap.h"

#include "vk_mesh_manager.h"
#include "core/vk_device.h"
#include "core/vk_buffer.h"
#include "core/vk_command_buffer.h"
#include "core/model/vk_model.h"

MeshManager::MeshManager(const Device* device, UploadContext* uploadContext) : _device(device), _uploadContext(uploadContext) {};

MeshManager::~MeshManager() {
    _entities.clear(); // _models.clear();

    if (_uploadContext->_commandPool != nullptr) {
        delete _uploadContext->_commandPool;
    }

}

void MeshManager::upload_mesh(Model& mesh) {
    size_t vertexBufferSize = mesh._verticesBuffer.size() * sizeof(Vertex);
    size_t indexBufferSize = mesh._indexesBuffer.size() * sizeof(uint32_t);
    mesh._indexBuffer.count = static_cast<uint32_t>(mesh._indexesBuffer.size());

    AllocatedBuffer vertexStaging = Buffer::create_buffer(*_device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    void* data;
    vmaMapMemory(_device->_allocator, vertexStaging._allocation, &data);
    memcpy(data, mesh._verticesBuffer.data(), static_cast<size_t>(vertexBufferSize)); // number of vertex
    vmaUnmapMemory(_device->_allocator, vertexStaging._allocation);

    mesh._vertexBuffer = Buffer::create_buffer(*_device, vertexBufferSize,  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);


    AllocatedBuffer indexStaging = Buffer::create_buffer(*_device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    void* data2;
    vmaMapMemory(_device->_allocator, indexStaging._allocation, &data2);
    memcpy(data2, mesh._indexesBuffer.data(), static_cast<size_t>(indexBufferSize));
    vmaUnmapMemory(_device->_allocator, indexStaging._allocation);

    mesh._indexBuffer.allocation = Buffer::create_buffer(*_device, indexBufferSize,   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    CommandBuffer::immediate_submit(*_device, *_uploadContext, [&](VkCommandBuffer cmd) {
        VkBufferCopy copy;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        copy.size = vertexBufferSize;
        vkCmdCopyBuffer(cmd, vertexStaging._buffer, mesh._vertexBuffer._buffer, 1, &copy);
    });

    CommandBuffer::immediate_submit(*_device, *_uploadContext, [&](VkCommandBuffer cmd) {
        VkBufferCopy copy;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        copy.size = indexBufferSize;
        vkCmdCopyBuffer(cmd, indexStaging._buffer, mesh._indexBuffer.allocation._buffer, 1, &copy);
    });

    vmaDestroyBuffer(_device->_allocator, vertexStaging._buffer, vertexStaging._allocation);
    vmaDestroyBuffer(_device->_allocator, indexStaging._buffer, indexStaging._allocation);
}

std::shared_ptr<Entity> MeshManager::get_model(const std::string &name) {
    return this->get_entity(name);
}