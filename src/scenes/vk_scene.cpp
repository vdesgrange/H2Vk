/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_scene.h"
#include "core/utilities/vk_global.h"
#include "components/camera/vk_camera.h"

void Scene::load_scene(int sceneIndex, Camera& camera) {
    if (sceneIndex == _sceneIndex) {
        return;
    }
    _ready = false;

    Camera adhocCamera{};
    auto renderables = SceneListing::scenes[sceneIndex].second(adhocCamera, &_engine);

    camera = adhocCamera;
    _sceneIndex = sceneIndex;
    _renderables.clear();
    _renderables = renderables;
    _ready = true;
}

void Scene::allocate_buffers(Device& device) {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        Buffer::create_buffer(device, &g_frames[i].objectBuffer, sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
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
        objectsBInfo.buffer = g_frames[i].objectBuffer._buffer;
        objectsBInfo.offset = 0;
        objectsBInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

        DescriptorBuilder::begin(layoutCache, allocator)
                .bind_buffer(objectsBInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
                .layout(setLayout)
                .build(g_frames[i].objectDescriptor, setLayout, poolSizes);
    }
}

void Scene::setup_texture_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout) {
    for (auto &renderable: this->_renderables) {
        renderable.model->setup_descriptors(layoutCache, allocator, setLayout); // Duplicate with scene listing. Not multi-thread safe. Use global _renderables.
    }
}

/**
 * @brief Render scene assets
 * @param commandBuffer
 */
void Scene::render_objects(VkCommandBuffer commandBuffer, FrameData& frame) {
    std::shared_ptr<Model> lastModel = nullptr;
    std::shared_ptr<Material> lastMaterial = nullptr;
    // uint32_t frameIndex = _frameNumber % FRAME_OVERLAP;

    uint32_t count = _renderables.size();
    RenderObject *first = _renderables.data();

    for (int i=0; i < count; i++) { // For each scene/object in the vector of scenes.
        RenderObject& object = first[i]; // Take the scene/object

        if (object.material != lastMaterial) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
            // uint32_t lightOffset = helper::pad_uniform_buffer_size(*_device,sizeof(GPULightData)) * frameIndex;
            std::vector<uint32_t> dynOffsets = {0, 0};
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &frame.environmentDescriptor, 2, dynOffsets.data());
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1, &frame.objectDescriptor, 0,nullptr);
        }

        if (object.model) {
            bool bind = object.model.get() != lastModel.get();
            object.model->draw(commandBuffer, object.material->pipelineLayout, sizeof(glm::mat4), i, bind);
            lastModel = bind ? object.model : lastModel;
        }
    }
}