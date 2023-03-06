#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include <iostream>

#include "vk_engine.h"

using namespace std;

void VulkanEngine::init()
{
    init_window();
    init_vulkan();
    init_swapchain();
    init_commands();
    init_default_renderpass();
    init_framebuffers();
    init_sync_structures();
	init_managers();
    init_interface();
    init_camera();
    init_scene();
    init_descriptors();
    init_pipelines();

    // update_uniform_buffers();
    update_buffer_objects(_scene->_renderables.data(), _scene->_renderables.size());
	_isInitialized = true;
}

void VulkanEngine::init_window() {
    _window = std::make_unique<Window>();
}

void VulkanEngine::init_vulkan() {
    _device = std::make_unique<Device>(*_window);
    std::cout << "GPU has a minimum buffer alignment = " << _device->_gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;
}

void VulkanEngine::init_interface() {
    Settings settings = Settings{.scene_index=0};
    _ui = std::make_unique<UInterface>(*this, settings);
    _ui->init_imgui();
}

void VulkanEngine::init_camera() {
    _camera = std::make_unique<Camera>();
    _camera->inverse(false);
    _camera->set_position({ 0.f, -6.f, -10.f });
    _camera->set_perspective(70.f, (float)_window->_windowExtent.width /(float)_window->_windowExtent.height, 0.1f, 200.0f);

    _window->on_get_key = [this](int key, int action) {
        bool updated = _camera->on_key(key, action);
//        if (updated) {
//            update_buffers();
//        }
    };

    _window->on_cursor_position = [this](double xpos, double ypos) {
        if (_ui->want_capture_mouse()) return;
        bool updated = _camera->on_cursor_position(xpos, ypos);
//        if (updated) {
//            update_uniform_buffers();
//        }
    };

    _window->on_mouse_button = [this](int button, int action, int mods) {
        if (_ui->want_capture_mouse()) return;
        bool updated = _camera->on_mouse_button(button, action, mods);
    };
}

void VulkanEngine::init_swapchain() {
    _swapchain = std::make_unique<SwapChain>(*_window, *_device);
}

void VulkanEngine::init_commands() {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        _frames[i]._commandPool = new CommandPool(*_device);
        _frames[i]._commandBuffer = new CommandBuffer(*_device, *_frames[i]._commandPool);
        _mainDeletionQueue.push_function([=]() {
            delete _frames[i]._commandPool;
        });
    }

    _uploadContext._commandPool = new CommandPool(*_device);
    _uploadContext._commandBuffer = new CommandBuffer(*_device, *_uploadContext._commandPool);
}

void VulkanEngine::init_default_renderpass() {
    _renderPass = std::make_unique<RenderPass>(*_device); // , *_swapchain

    RenderPass::Attachment color = _renderPass->color(_swapchain->_swapChainImageFormat);
    RenderPass::Attachment depth = _renderPass->depth(_swapchain->_depthFormat);
    VkSubpassDescription subpass = _renderPass->subpass_description(&color.ref, &depth.ref);
    std::vector<VkAttachmentDescription> attachments = {color.description, depth.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency, depth.dependency};

    _renderPass->init(attachments, dependencies, subpass);
}

void VulkanEngine::init_framebuffers() {
    _frameBuffers = std::make_unique<FrameBuffers>(*_window, *_device, *_swapchain, *_renderPass);
}

void VulkanEngine::init_sync_structures() {
    //  Used for GPU -> GPU synchronisation
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        _frames[i]._renderFence = new Fence(*_device, VK_FENCE_CREATE_SIGNALED_BIT);
        _frames[i]._renderSemaphore = new Semaphore(*_device);
        _frames[i]._presentSemaphore =  new Semaphore(*_device);

        _mainDeletionQueue.push_function([=]() { // Destruction of semaphores
            delete _frames[i]._presentSemaphore;
            delete _frames[i]._renderSemaphore;
            delete _frames[i]._renderFence;
        });
    }

    _uploadContext._uploadFence = new Fence(*_device);
    _mainDeletionQueue.push_function([=]() {
        delete _uploadContext._uploadFence;
    });
}

void VulkanEngine::init_descriptors() {
    std::vector<VkDescriptorPoolSize> skyboxPoolSizes = {
           {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}
    };

    std::vector<VkDescriptorPoolSize> poolSizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20 }
    };

    _layoutCache = new DescriptorLayoutCache(*_device);
    _allocator = new DescriptorAllocator(*_device);

    // === Environment 1 ===
//    const size_t sceneParamBufferSize = FRAME_OVERLAP * Helper::pad_uniform_buffer_size(*_device, sizeof(GPUSceneData));
//    _sceneParameterBuffer = Buffer::create_buffer(*_device, sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
//    VkDescriptorBufferInfo sceneBInfo{};
//    sceneBInfo.buffer = _sceneParameterBuffer._buffer;
//    sceneBInfo.range = sizeof(GPUSceneData);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        // === Camera ===
        _frames[i].skyboxDescriptor = VkDescriptorSet();
        _frames[i].environmentDescriptor = VkDescriptorSet();
        _frames[i].cameraBuffer = Buffer::create_buffer(*_device, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo camBInfo{};
        camBInfo.buffer = _frames[i].cameraBuffer._buffer;
        camBInfo.offset = 0;
        camBInfo.range = sizeof(GPUCameraData);

        // === Light ===
        const size_t lightBufferSize = FRAME_OVERLAP * Helper::pad_uniform_buffer_size(*_device, sizeof(GPULightData));
        _frames[i].lightingBuffer = Buffer::create_buffer(*_device, lightBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        VkDescriptorBufferInfo lightingBInfo{};
        lightingBInfo.buffer = _frames[i].lightingBuffer._buffer;
        lightingBInfo.range = sizeof(GPULightData);

        DescriptorBuilder::begin(*_layoutCache, *_allocator)
                .bind_buffer(camBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
                .bind_buffer(lightingBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                .bind_image(_skybox->_environment._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)
                .bind_image(_skybox->_prefilter._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
                .bind_image(_skybox->_brdf._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4)
                .layout(_descriptorSetLayouts.environment) // use reference instead?
                .build(_frames[i].environmentDescriptor, _descriptorSetLayouts.environment, poolSizes);

        // === Skybox === (Build by default to handle if skybox enabled later)
        DescriptorBuilder::begin(*_layoutCache, *_allocator)
            .bind_buffer(camBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
            .bind_image(_skybox->_background._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
            .layout(_descriptorSetLayouts.skybox)
            .build(_frames[i].skyboxDescriptor, _descriptorSetLayouts.skybox, skyboxPoolSizes);

        // === Object ===
        const uint32_t MAX_OBJECTS = 10000;
        _frames[i].objectDescriptor = VkDescriptorSet();
        _frames[i].objectBuffer = Buffer::create_buffer(*_device, sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo objectsBInfo{};
        objectsBInfo.buffer = _frames[i].objectBuffer._buffer;
        objectsBInfo.offset = 0;
        objectsBInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

        DescriptorBuilder::begin(*_layoutCache, *_allocator)
            .bind_buffer(objectsBInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
            .layout(_descriptorSetLayouts.matrices) // use reference instead?
            .build(_frames[i].objectDescriptor, _descriptorSetLayouts.matrices, poolSizes);
    }

    // === Texture ===
    DescriptorBuilder::begin(*_layoutCache, *_allocator)
            .bind_none(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
            .bind_none(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
            .bind_none(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)
            .bind_none(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
            .bind_none(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4)
            .layout(_descriptorSetLayouts.textures);

    // === Clean up ===
    _mainDeletionQueue.push_function([&]() {
        // vmaDestroyBuffer(_device->_allocator, _sceneParameterBuffer._buffer, _sceneParameterBuffer._allocation);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vmaDestroyBuffer(_device->_allocator, _frames[i].cameraBuffer._buffer, _frames[i].cameraBuffer._allocation);
            vmaDestroyBuffer(_device->_allocator, _frames[i].lightingBuffer._buffer, _frames[i].lightingBuffer._allocation);
            vmaDestroyBuffer(_device->_allocator, _frames[i].objectBuffer._buffer, _frames[i].objectBuffer._allocation);
        }

        delete _layoutCache;
        delete _allocator;
    });
}

void VulkanEngine::setup_descriptors(){
    for (auto &renderable: _scene->_renderables) {
        std::vector<VkDescriptorPoolSize> poolSizes = {
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>( renderable.model->_images.size() ) }
        };

        for (auto &material: renderable.model->_materials) {
            if (material.pbr == false) {
                VkDescriptorImageInfo colorMap = renderable.model->_images[material.baseColorTextureIndex]._texture._descriptor;
                VkDescriptorImageInfo normalMap = renderable.model->_images[material.normalTextureIndex]._texture._descriptor;
                VkDescriptorImageInfo metallicRoughnessMap = renderable.model->_images[material.metallicRoughnessTextureIndex]._texture._descriptor;
                VkDescriptorImageInfo aoMap = renderable.model->_images[material.aoTextureIndex]._texture._descriptor;
                VkDescriptorImageInfo emissiveMap = renderable.model->_images[material.emissiveTextureIndex]._texture._descriptor;

                DescriptorBuilder::begin(*_layoutCache, *_allocator)
                        .bind_image(colorMap, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,0)
                        .bind_image(normalMap, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,1)
                        .bind_image(metallicRoughnessMap, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,2)
                        .bind_image(aoMap, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,3)
                        .bind_image(emissiveMap, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,4)
                        .layout(_descriptorSetLayouts.textures)
                        .build(material._descriptorSet,_descriptorSetLayouts.textures, poolSizes); // _images[material.baseColorTextureIndex]._descriptorSet
            }
        }
    }
}

void VulkanEngine::init_pipelines() {
    _pipelineBuilder = std::make_unique<PipelineBuilder>(*_device, *_renderPass);

    std::vector<VkDescriptorSetLayout> setLayouts = {_descriptorSetLayouts.environment, _descriptorSetLayouts.matrices, _descriptorSetLayouts.textures};
    _pipelineBuilder->scene_light(setLayouts);
    _pipelineBuilder->scene_spheres(setLayouts);
    _pipelineBuilder->scene_damaged_helmet(setLayouts);

    // === Skybox === (Build by default to handle if skybox enabled later)
    _skybox->setup_pipeline(*_pipelineBuilder, {_descriptorSetLayouts.skybox});
}

FrameData& VulkanEngine::get_current_frame() {
    return _frames[_frameNumber % FRAME_OVERLAP];
}

void VulkanEngine::init_managers() {
    _meshManager = std::make_unique<MeshManager>(*_device, _uploadContext); // move to sceneListing ?
    _textureManager = std::make_unique<TextureManager>(*this);
    _samplerManager = std::make_unique<SamplerManager>(*this);
}

void VulkanEngine::init_scene() {
    _skybox = std::make_unique<Skybox>(*_device, *_pipelineBuilder, *_textureManager, *_meshManager, _uploadContext);
    _skybox->_type = Skybox::Type::box;
    _skybox->load();

    _sceneListing = std::make_unique<SceneListing>();
    _scene = std::make_unique<Scene>(*this);
}

void VulkanEngine::recreate_swap_chain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(_window->_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window->_window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(_device->_logicalDevice);
    _frameBuffers.reset();
    _renderPass.reset();
    _swapchain.reset();

    // Command buffers
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        delete _frames[i]._commandBuffer;
        _frames[i]._commandBuffer = new CommandBuffer(*_device, *_frames[i]._commandPool);
    }
    delete _uploadContext._commandBuffer;
    _uploadContext._commandBuffer = new CommandBuffer(*_device, *_uploadContext._commandPool);

    init_swapchain();
    init_default_renderpass();
    init_framebuffers();

    if (_window->_windowExtent.width > 0.0f && _window->_windowExtent.height > 0.0f) {
        // _camera->set_aspect((float)_window->_windowExtent.width /(float)_window->_windowExtent.height);
        _camera->set_aspect((float)_window->_windowExtent.width / (float)_window->_windowExtent.height);
        update_uniform_buffers();
    }

}

Statistics VulkanEngine::monitoring() {
    // Record delta time between calls to Render.
    const auto prevTime = _time;
    _time = _window->get_time();
    const auto timeDelta = _time - prevTime;

    Statistics stats = {};
    stats.FramebufferSize = _window->get_framebuffer_size();
    stats.FrameRate = static_cast<float>(1 / timeDelta); // FPS
    stats.coordinates[0] = _camera->get_position_vector().x;
    stats.coordinates[1] = _camera->get_position_vector().y;
    stats.coordinates[2] = _camera->get_position_vector().z;
    return stats;
}

void VulkanEngine::ui_overlay() {
    Statistics stats = monitoring();

    bool updated = _ui->render(get_current_frame()._commandBuffer->_commandBuffer, stats);
    if (updated) {
        // Camera
        // _camera->set_type(static_cast<Camera::Type>(_ui->get_settings().type));
        // _camera->set_speed(_ui->get_settings().speed);
        // _camera->set_perspective(_ui->get_settings().angle, _camera->get_aspect(), _ui->get_settings().zNearFar[0],_ui->get_settings().zNearFar[1]);
        // _camera->set_target({_ui->get_settings().target[0], _ui->get_settings().target[1], _ui->get_settings().target[2]});

        // Scene
//        _sceneParameters.sunlightColor = {_ui->get_settings().colors[0], _ui->get_settings().colors[1], _ui->get_settings().colors[2], 1.0};
//        _sceneParameters.sunlightDirection = {_ui->get_settings().coordinates[0], _ui->get_settings().coordinates[1], _ui->get_settings().coordinates[2], _ui->get_settings().ambient};
//        _sceneParameters.specularFactor = _ui->get_settings().specular;

        // Skybox
        _skyboxDisplay = _ui->p_open[SKYBOX_EDITOR];
    }

    // Move camera when key press
    _reset = _camera->update_camera(1. / stats.FrameRate);
}

void VulkanEngine::skybox(VkCommandBuffer commandBuffer) {
    if (_skyboxDisplay) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _skybox->_material->pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _skybox->_material->pipelineLayout, 0,1, &get_current_frame().skyboxDescriptor, 0, nullptr);
        _skybox->draw(commandBuffer);
    }
}

void VulkanEngine::update_uniform_buffers() {
    FrameData frame =  get_current_frame(); // _frames[i];
    uint32_t frameIndex = _frameNumber % FRAME_OVERLAP; // i;

    // === Camera & Objects & Environment ===
    GPUCameraData camData{};
    camData.proj = _camera->get_projection_matrix();
    camData.view = _camera->get_view_matrix();
    camData.pos = _camera->get_position_vector();
    camData.flip = _camera->get_flip();

    // Camera : write into the buffer by copying the render matrices from camera object into it
    void *data;
    vmaMapMemory(_device->_allocator, frame.cameraBuffer._allocation, &data);
    memcpy(data, &camData, sizeof(GPUCameraData));
    vmaUnmapMemory(_device->_allocator, frame.cameraBuffer._allocation);

    // Light : write scene data into lighting buffer
    // === Camera & Objects & Environment ===
    GPULightData lightingData{};
    lightingData.num_lights = static_cast<uint32_t>(_lights.size());
    int lightCount = 0;
    for (const auto& light : _lights) {
        lightingData.position[lightCount] = light._position; // glm::vec4(0.0f, 0.0f, 5.0f, 0.0f);
        lightingData.color[lightCount] = light._color; // glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        lightCount++;
    }
//    lightingData.position[0] = glm::vec4(0.0f, 0.0f, 5.0f, 0.0f);
//    lightingData.color[0] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    char *data2;
    vmaMapMemory(_device->_allocator, frame.lightingBuffer._allocation,   (void **) &data2);
    data2 += Helper::pad_uniform_buffer_size(*_device, sizeof(GPULightData)) * frameIndex;
    memcpy(data2, &lightingData, sizeof(GPULightData));
    vmaUnmapMemory(_device->_allocator, frame.lightingBuffer._allocation);

    // Environment : write scene data into environment buffer
//    char *sceneData;
//    vmaMapMemory(_device->_allocator, _sceneParameterBuffer._allocation, (void **) &sceneData);
//    sceneData += Helper::pad_uniform_buffer_size(*_device, sizeof(GPUSceneData)) * frameIndex;
//    memcpy(sceneData, &_sceneParameters, sizeof(GPUSceneData));
//    vmaUnmapMemory(_device->_allocator, _sceneParameterBuffer._allocation);

}

void VulkanEngine::update_buffer_objects(RenderObject *first, int count) {
    // Objects : write into the buffer by copying the render matrices from our render objects into it
    // Object not moving : call only when change scene
    for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
        FrameData frame = _frames[i]; // get_current_frame(); // if object moves, call each frame.
        void* objectData;
        vmaMapMemory(_device->_allocator, frame.objectBuffer._allocation, &objectData);
        GPUObjectData* objectSSBO = (GPUObjectData*)objectData;
        for (int i = 0; i < count; i++) {
            RenderObject& object = first[i];
            objectSSBO[i].model = object.transformMatrix;
        }
        vmaUnmapMemory(_device->_allocator, frame.objectBuffer._allocation);
    }
}

void VulkanEngine::render_objects(VkCommandBuffer commandBuffer) {
    std::shared_ptr<Model> lastModel = nullptr;
    std::shared_ptr<Material> lastMaterial = nullptr;
    int frameIndex = _frameNumber % FRAME_OVERLAP;
    int count = _scene->_renderables.size();
    RenderObject *first = _scene->_renderables.data();

    // === Update required uniform buffers
    update_uniform_buffers(); // If called at every frame: fix the position jump of the camera when moving
    // update_buffer_objects(first, count);

    // === Bind ===
    skybox(commandBuffer);

    for (int i=0; i < count; i++) { // For each scene/object in the vector of scenes.
        RenderObject& object = first[i]; // Take the scene/object

        if (object.material != lastMaterial) { // Same material = (shaders/pipeline/descriptors) for multiple objects part of the same scene (e.g. monkey + triangles)
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
            uint32_t dynOffset = Helper::pad_uniform_buffer_size(*_device,sizeof(GPULightData)) * frameIndex; // GPUSceneData
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &get_current_frame().environmentDescriptor, 1, &dynOffset);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1, &get_current_frame().objectDescriptor, 0,nullptr);
        }

        if (object.model) {
            bool bind = object.model.get() != lastModel.get(); // degueulasse sortir de la boucle de rendu
            object.model->draw(commandBuffer, object.material->pipelineLayout, i, object.model != lastModel);
            lastModel = bind ? object.model : lastModel;
        }
    }
}

void VulkanEngine::build_command_buffers(FrameData frame, int imageIndex) {
    // Record command buffers
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil.depth = 1.0f;

    // === Render pass ====
    // Collection of attachments, subpasses, and dependencies between the subpasses
    VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(_renderPass->_renderPass,
                                                                         _window->_windowExtent,
                                                                         _frameBuffers->_frameBuffers[imageIndex]);
    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();


    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(frame._commandBuffer->_commandBuffer, &cmdBeginInfo));
    {
        vkCmdBeginRenderPass(frame._commandBuffer->_commandBuffer, &renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);
        {
            VkViewport viewport = vkinit::get_viewport((float) _window->_windowExtent.width, (float) _window->_windowExtent.height);
            vkCmdSetViewport(frame._commandBuffer->_commandBuffer, 0, 1, &viewport);

            VkRect2D scissor = vkinit::get_scissor((float) _window->_windowExtent.width, (float) _window->_windowExtent.height);
            vkCmdSetScissor(frame._commandBuffer->_commandBuffer, 0, 1, &scissor);

            // Scene
            if (_scene->_sceneIndex != _ui->get_settings().scene_index) {
                _scene->load_scene(_ui->get_settings().scene_index, *_camera);
                this->setup_descriptors();
                // update_uniform_buffers(); // Re-initialize position after scene change = camera jumping.
                this->update_buffer_objects(_scene->_renderables.data(), _scene->_renderables.size());
            }
            // Draw
            this->render_objects(frame._commandBuffer->_commandBuffer);

            this->ui_overlay();
        }
        vkCmdEndRenderPass(get_current_frame()._commandBuffer->_commandBuffer);

    }
    VK_CHECK(vkEndCommandBuffer(get_current_frame()._commandBuffer->_commandBuffer));

}

void VulkanEngine::render(int imageIndex) {
    // === Command Buffer ===
    VK_CHECK(vkResetCommandBuffer(get_current_frame()._commandBuffer->_commandBuffer, 0));
    this->build_command_buffers(get_current_frame(), imageIndex);

    // --- Submit queue
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.waitSemaphoreCount = 1; // Semaphore to wait before executing the command buffers
    submitInfo.pWaitSemaphores = &(get_current_frame()._presentSemaphore->_semaphore);
    submitInfo.signalSemaphoreCount = 1; // Number of semaphores to be signaled once the commands
    submitInfo.pSignalSemaphores = &(get_current_frame()._renderSemaphore->_semaphore);
    submitInfo.commandBufferCount = 1; // Number of command buffers to execute in the batch
    submitInfo.pCommandBuffers = &get_current_frame()._commandBuffer->_commandBuffer;

    VK_CHECK(vkQueueSubmit(_device->get_graphics_queue(), 1, &submitInfo, get_current_frame()._renderFence->_fence));
}

void VulkanEngine::draw() { // todo : what need to be called at each frame? draw()? commandBuffers?
    // === Prepare frame ===
    // Wait GPU to render latest frame. Timeout of 1 second
     VK_CHECK(get_current_frame()._renderFence->wait(1000000000));
     VK_CHECK(get_current_frame()._renderFence->reset());

    // Acquire next image
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_device->_logicalDevice, _swapchain->_swapchain, 1000000000, get_current_frame()._presentSemaphore->_semaphore, nullptr, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swap_chain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        throw std::runtime_error("failed to acquire swap chain image");
    }

    // === Rendering commands
    render(imageIndex);

    // === Submit frame ===
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &(_swapchain->_swapchain);
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &(get_current_frame()._renderSemaphore->_semaphore);

    result = vkQueuePresentKHR(_device->get_graphics_queue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _window->_framebufferResized) {
        _window->_framebufferResized = false;
        recreate_swap_chain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    _frameNumber++;
}

void VulkanEngine::run()
{
    while(!glfwWindowShouldClose(_window->_window)) {
        glfwPollEvents();
        Window::glfw_get_key(_window->_window);
        draw();
    }

    vkDeviceWaitIdle(_device->_logicalDevice);
}

void VulkanEngine::cleanup()
{
    if (_isInitialized) {
        vkDeviceWaitIdle(_device->_logicalDevice);
        for (int i=0; i < FRAME_OVERLAP; i++) {
            _frames[i]._renderFence->wait(1000000000);
        }

        _scene->_renderables.clear();
        _skybox.reset();
        _ui.reset();
        _samplerManager.reset();
        _textureManager.reset();
        _meshManager.reset();
        _pipelineBuilder.reset();
        _frameBuffers.reset();
        _renderPass.reset();
        _swapchain.reset();
        _mainDeletionQueue.flush();

        // todo find a way to move this into vk_device without breaking swapchain
        vmaDestroyAllocator(_device->_allocator);
        vkDestroyDevice(_device->_logicalDevice, nullptr);
        vkDestroySurfaceKHR(_device->_instance, _device->_surface, nullptr);
        vkb::destroy_debug_utils_messenger(_device->_instance, _device->_debug_messenger);
        vkDestroyInstance(_device->_instance, nullptr);

        _device.reset();
        _window.reset();
        glfwTerminate();
    }
}
