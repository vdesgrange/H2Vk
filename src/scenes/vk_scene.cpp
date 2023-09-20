/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_scene.h"
#include "core/utilities/vk_global.h"

void Scene::load_scene(int sceneIndex, Camera& camera) {
    if (sceneIndex == _sceneIndex) {
        return;
    }

    _renderables.clear();
    auto renderables = SceneListing::scenes[sceneIndex].second(camera, &_engine);
    _sceneIndex = sceneIndex;
    _renderables = renderables;
}

void Scene::allocate_buffers(Device& device) {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        g_frames[i].objectBuffer = Buffer::create_buffer(device, sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }
}

void Scene::setup_transformation_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout) {
    std::vector<VkDescriptorPoolSize> poolSizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        g_frames[i].objectDescriptor = VkDescriptorSet();

        VkDescriptorBufferInfo objectsBInfo{};
        objectsBInfo.buffer = g_frames[i].objectBuffer._buffer; // allocated once
        objectsBInfo.offset = 0;
        objectsBInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

        DescriptorBuilder::begin(layoutCache, allocator)
                .bind_buffer(objectsBInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
                .layout(setLayout)
                .build(g_frames[i].objectDescriptor, setLayout, poolSizes);
    }
}

void Scene::setup_texture_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout, Texture& emptyTexture) {
    for (auto &renderable: this->_renderables) {
        renderable.model->setup_descriptors(layoutCache, allocator, setLayout,emptyTexture);
    }
}