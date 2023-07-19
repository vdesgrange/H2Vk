/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "glm/glm.hpp"
#include "core/vk_shaders.h"
#include "core/vk_buffer.h"

#include <deque>
#include <functional>

class Model;
class Semaphore;
class Fence;
class CommandPool;
class CommandBuffer;
class DescriptorLayoutCache;
class DescriptorAllocator;

struct GPUObjectData {
    glm::mat4 model;
};

struct RenderObject {
    std::shared_ptr<Model> model;
    std::shared_ptr<Material> material;
    glm::mat4 transformMatrix;
};

typedef std::vector<RenderObject> Renderables;

struct FrameData {
    Semaphore* _presentSemaphore;
    Semaphore* _renderSemaphore;
    Fence* _renderFence;

    CommandPool* _commandPool;
    CommandBuffer* _commandBuffer;

    VkDescriptorSet skyboxDescriptor;

    AllocatedBuffer cameraBuffer;
    AllocatedBuffer lightingBuffer;
    VkDescriptorSet environmentDescriptor;

    AllocatedBuffer objectBuffer;
    VkDescriptorSet objectDescriptor;

    AllocatedBuffer offscreenBuffer;
    VkDescriptorSet offscreenDescriptor;

    VkDescriptorSet debugDescriptor;
};

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)(); // call the function
        }

        deletors.clear();
    }
};