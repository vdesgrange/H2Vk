#include "VkBootstrap.h"

#include "vk_mesh_manager.h"
#include "vk_helpers.h"
#include "vk_device.h"
#include "core/model/vk_mesh.h"
#include "vk_buffer.h"
#include "vk_initializers.h"
#include "vk_fence.h"
#include "vk_command_buffer.h"
#include "vk_command_pool.h"

MeshManager::MeshManager(const Device& device, UploadContext& uploadContext) : _device(device), _uploadContext(uploadContext) {};

MeshManager::~MeshManager() {
//    for (auto& it: _meshes) {
//        vmaDestroyBuffer(_device._allocator,
//                         it.second._vertexBuffer._buffer,
//                         it.second._vertexBuffer._allocation);
//    }

    for (auto& it: _models) {
        for (auto node : it.second._nodes) {
            delete node;
        }

        for (Image image : it.second._images) {
            vkDestroyImageView(_device._logicalDevice, image._texture._imageView, nullptr);
            vmaDestroyImage(_device._allocator, image._texture._image, image._texture._allocation);  // destroyImage + vmaFreeMemory
            vkDestroySampler(_device._logicalDevice, image._texture._sampler, nullptr);
        }

        vmaDestroyBuffer(_device._allocator, it.second._vertexBuffer._buffer, it.second._vertexBuffer._allocation);
        vmaDestroyBuffer(_device._allocator, it.second._indexBuffer.allocation._buffer, it.second._indexBuffer.allocation._allocation);
    }

    if (_uploadContext._commandPool != nullptr) {
        delete _uploadContext._commandPool;
    }

}

void MeshManager::upload_mesh(Model& mesh) {
    size_t vertexBufferSize = mesh._verticesBuffer.size() * sizeof(Vertex);
    size_t indexBufferSize = mesh._indexesBuffer.size() * sizeof(uint32_t);
    mesh._indexBuffer.count = static_cast<uint32_t>(mesh._indexesBuffer.size());

    AllocatedBuffer vertexStaging = Buffer::create_buffer(_device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    void* data;
    vmaMapMemory(_device._allocator, vertexStaging._allocation, &data);
    memcpy(data, mesh._verticesBuffer.data(), static_cast<size_t>(vertexBufferSize)); // number of vertex
    vmaUnmapMemory(_device._allocator, vertexStaging._allocation);
    mesh._vertexBuffer = Buffer::create_buffer(_device, vertexBufferSize,  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    immediate_submit([=](VkCommandBuffer cmd) {
        VkBufferCopy copy;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        copy.size = vertexBufferSize;
        vkCmdCopyBuffer(cmd, vertexStaging._buffer, mesh._vertexBuffer._buffer, 1, &copy);
    });

    AllocatedBuffer indexStaging = Buffer::create_buffer(_device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    void* data2;
    vmaMapMemory(_device._allocator, indexStaging._allocation, &data2);
    memcpy(data2, mesh._indexesBuffer.data(), static_cast<size_t>(indexBufferSize));
    vmaUnmapMemory(_device._allocator, indexStaging._allocation);
    mesh._indexBuffer.allocation = Buffer::create_buffer(_device, indexBufferSize,   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    immediate_submit([=](VkCommandBuffer cmd) {
        VkBufferCopy copy;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        copy.size = indexBufferSize;
        vkCmdCopyBuffer(cmd, indexStaging._buffer, mesh._indexBuffer.allocation._buffer, 1, &copy);
    });

    vmaDestroyBuffer(_device._allocator, vertexStaging._buffer, vertexStaging._allocation);
    vmaDestroyBuffer(_device._allocator, indexStaging._buffer, indexStaging._allocation);
}

Model* MeshManager::get_model(const std::string &name) {
    auto it = _models.find(name);
    if ( it == _models.end()) {
        return nullptr;
    } else {
        return &(*it).second;
        // return it->second.get();
    }
}

Mesh* MeshManager::get_mesh(const std::string &name) {
    auto it = _meshes.find(name);
    if ( it == _meshes.end()) {
        return nullptr;
    } else {
        return &(*it).second;
    }
}

void MeshManager::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) {
    VkCommandBuffer cmd = _uploadContext._commandBuffer->_mainCommandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    function(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submitInfo = vkinit::submit_info(&cmd);
    VK_CHECK(vkQueueSubmit(_device.get_graphics_queue(), 1, &submitInfo, _uploadContext._uploadFence->_fence));

    vkWaitForFences(_device._logicalDevice, 1, &_uploadContext._uploadFence->_fence, true, 9999999999);
    vkResetFences(_device._logicalDevice, 1, &_uploadContext._uploadFence->_fence);

    vkResetCommandPool(_device._logicalDevice, _uploadContext._commandPool->_commandPool, 0);
}
