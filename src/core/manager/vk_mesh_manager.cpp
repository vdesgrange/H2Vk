/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "VkBootstrap.h"

#include "vk_mesh_manager.h"
#include "core/vk_device.h"
#include "core/vk_buffer.h"
#include "core/vk_command_buffer.h"
#include "components/model/vk_model.h"

MeshManager::MeshManager(const Device* device, UploadContext* uploadContext) : _device(device), _uploadContext(uploadContext) {};

MeshManager::~MeshManager() {
    _entities.clear();

//    if (_uploadContext->_commandPool != nullptr) {
//        delete _uploadContext->_commandPool;
//    }

}

void MeshManager::upload_mesh(Model& mesh) {
    size_t vertexBufferSize = mesh._verticesBuffer.size() * sizeof(Vertex);
    size_t indexBufferSize = mesh._indexesBuffer.size() * sizeof(uint32_t);
    mesh._indexBuffer.count = static_cast<uint32_t>(mesh._indexesBuffer.size());

    AllocatedBuffer vertexStaging;
    Buffer::create_buffer(*_device, &vertexStaging, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    vertexStaging.map();
    vertexStaging.copyFrom(mesh._verticesBuffer.data(), static_cast<size_t>(vertexBufferSize));
    vertexStaging.unmap();

    Buffer::create_buffer(*_device, &mesh._vertexBuffer, vertexBufferSize,  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    AllocatedBuffer indexStaging;
    Buffer::create_buffer(*_device, &indexStaging, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    indexStaging.map();
    indexStaging.copyFrom(mesh._indexesBuffer.data(), static_cast<size_t>(indexBufferSize));
    indexStaging.unmap();

    Buffer::create_buffer(*_device, &mesh._indexBuffer.allocation, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

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

    // vertexStaging.destroy();
    // indexStaging.destroy();
}

std::shared_ptr<Model> MeshManager::get_model(const std::string &name) {
    return std::static_pointer_cast<Model>(this->get_entity(name));
}