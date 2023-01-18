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
	load_meshes();
    load_images();
    init_interface();
    init_camera();
    init_scene();
    init_descriptors();
    init_pipelines();

	_isInitialized = true;
}

void VulkanEngine::init_window() {
    _window = std::make_unique<class Window>();
}

void VulkanEngine::init_vulkan() {
    _device = std::make_unique<class Device>(*_window);
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
    _camera->set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    _window->on_get_key = [this](int key, int action) {
        _camera->on_key(key, action);
    };

    _window->on_cursor_position = [this](double xpos, double ypos) {
        if (_ui->want_capture_mouse()) return;
        _camera->on_cursor_position(xpos, ypos);
    };

    _window->on_mouse_button = [this](int button, int action, int mods) {
        if (_ui->want_capture_mouse()) return;
        _camera->on_mouse_button(button, action, mods);
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
    _renderPass = std::make_unique<class RenderPass>(*_device, *_swapchain);
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
           {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2}
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
    const size_t sceneParamBufferSize = FRAME_OVERLAP * Helper::pad_uniform_buffer_size(*_device, sizeof(GPUSceneData));
    _sceneParameterBuffer = Buffer::create_buffer(*_device, sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    VkDescriptorBufferInfo sceneBInfo{};
    sceneBInfo.buffer = _sceneParameterBuffer._buffer;
    sceneBInfo.range = sizeof(GPUSceneData);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        // === Environment 2 ===
        _frames[i].skyboxDescriptor = VkDescriptorSet();
        _frames[i].environmentDescriptor = VkDescriptorSet();
        _frames[i].cameraBuffer = Buffer::create_buffer(*_device, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo camBInfo{};
        camBInfo.buffer = _frames[i].cameraBuffer._buffer;
        camBInfo.offset = 0;
        camBInfo.range = sizeof(GPUCameraData);

        DescriptorBuilder::begin(*_layoutCache, *_allocator)
                .bind_buffer(camBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
                .bind_buffer(sceneBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                .layout(_descriptorSetLayouts.environment) // use reference instead?
                .build(_frames[i].environmentDescriptor, _descriptorSetLayouts.environment, poolSizes);

        // === Skybox === (Build by default to handle if skybox enabled later)
        DescriptorBuilder::begin(*_layoutCache, *_allocator)
            .bind_buffer(camBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
            .bind_image(_skybox->_texture._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
            .layout(_descriptorSetLayouts.skybox)
            .build(_frames[i].skyboxDescriptor, _descriptorSetLayouts.skybox, skyboxPoolSizes);

        // === Object ===
        const uint32_t MAX_OBJECTS = 10000;
        _frames[i].objectDescriptor = VkDescriptorSet();
        _frames[i].objectBuffer = Buffer::create_buffer(*_device, sizeof(GPUCameraData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

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
            .layout(_descriptorSetLayouts.textures);

    // === Clean up ===
    _mainDeletionQueue.push_function([&]() {
        vmaDestroyBuffer(_device->_allocator, _sceneParameterBuffer._buffer, _sceneParameterBuffer._allocation);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vmaDestroyBuffer(_device->_allocator, _frames[i].cameraBuffer._buffer, _frames[i].cameraBuffer._allocation);
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
            VkDescriptorImageInfo colorMap =  renderable.model->_images[material.baseColorTextureIndex]._texture._descriptor;
            VkDescriptorImageInfo normalMap =  renderable.model->_images[material.normalTextureIndex]._texture._descriptor;

            DescriptorBuilder::begin(*_layoutCache, *_allocator)
            .bind_image(colorMap, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
            .bind_image(normalMap, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
            .layout(_descriptorSetLayouts.textures)
            .build(renderable.model->_images[material.baseColorTextureIndex]._descriptorSet, _descriptorSetLayouts.textures, poolSizes);
        }
    }
}

void VulkanEngine::init_pipelines() {
    _pipelineBuilder = std::make_unique<PipelineBuilder>(*_window, *_device, *_renderPass);

    std::vector<VkDescriptorSetLayout> setLayouts = {_descriptorSetLayouts.environment, _descriptorSetLayouts.matrices, _descriptorSetLayouts.textures};
    _pipelineBuilder->scene_light(setLayouts);
    _pipelineBuilder->scene_monkey_triangle(setLayouts);
    _pipelineBuilder->scene_karibu_hippo(setLayouts);
    _pipelineBuilder->scene_damaged_helmet(setLayouts);

    // === Skybox === (Build by default to handle if skybox enabled later)
    _pipelineBuilder->skybox({_descriptorSetLayouts.skybox});
    _skybox->_material = this->_pipelineBuilder->get_material("skyboxMaterial");
}

FrameData& VulkanEngine::get_current_frame() {
    return _frames[_frameNumber % FRAME_OVERLAP];
}

void VulkanEngine::load_meshes() {
    _meshManager = std::make_unique<MeshManager>(*_device, _uploadContext); // move to sceneListing ?
}

void VulkanEngine::load_images() {
    _textureManager = std::make_unique<TextureManager>(*this);
    _samplerManager = std::make_unique<SamplerManager>(*this);
}

void VulkanEngine::init_scene() {
    _skybox = std::make_unique<Skybox>(*_device, *_pipelineBuilder, *_textureManager, *_meshManager, _uploadContext);
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

    _scene->_renderables.clear(); // Problem: Must update pipeline, however should not have to delete objects.
    _pipelineBuilder.reset(); // Revoir comment gerer pipeline avec scene.
    _frameBuffers.reset();
    _renderPass.reset();
    _swapchain.reset();

    init_swapchain();
    init_default_renderpass();
    init_framebuffers();
    // init_scene(); // Should not have to initialized fully the scene
    init_pipelines();
}

void VulkanEngine::skybox(VkCommandBuffer commandBuffer) {
    if (_skyboxDisplay) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _skybox->_material->pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _skybox->_material->pipelineLayout, 0,1, &get_current_frame().skyboxDescriptor, 0, nullptr);
        _skybox->_cube->draw(commandBuffer, _skybox->_material->pipelineLayout, 0, true);
    }

}

void VulkanEngine::draw_objects(VkCommandBuffer commandBuffer, RenderObject *first, int count) {
    std::shared_ptr<Model> lastModel = nullptr;
    std::shared_ptr<Material> lastMaterial = nullptr;

    // === Camera & Objects & Environment ===
    GPUCameraData camData{};
    camData.proj = _camera->get_projection_matrix();
    camData.view = _camera->get_view_matrix();
    camData.pos = _camera->get_position_vector();

    // Camera : write into the buffer by copying the render matrices from camera object into it
    void* data;
    vmaMapMemory(_device->_allocator, get_current_frame().cameraBuffer._allocation, &data);
    memcpy(data, &camData, sizeof(GPUCameraData));
    vmaUnmapMemory(_device->_allocator, get_current_frame().cameraBuffer._allocation);

    // Environment : write scene data into environment buffer
    char* sceneData;
    vmaMapMemory(_device->_allocator, _sceneParameterBuffer._allocation , (void**)&sceneData);
    int frameIndex = _frameNumber % FRAME_OVERLAP;
    sceneData += Helper::pad_uniform_buffer_size(*_device,sizeof(GPUSceneData)) * frameIndex;
    memcpy(sceneData, &_sceneParameters, sizeof(GPUSceneData));
    vmaUnmapMemory(_device->_allocator, _sceneParameterBuffer._allocation);

    // Objects : write into the buffer by copying the render matrices from our render objects into it
    void* objectData;
    vmaMapMemory(_device->_allocator, get_current_frame().objectBuffer._allocation, &objectData);
    GPUObjectData* objectSSBO = (GPUObjectData*)objectData;
    for (int i = 0; i < count; i++) {
        RenderObject& object = first[i];
        objectSSBO[i].model = object.transformMatrix;
    }
    vmaUnmapMemory(_device->_allocator, get_current_frame().objectBuffer._allocation);

    // Skybox
    skybox(commandBuffer);

    // === Drawing ===
    for (int i=0; i < count; i++) { // For each scene/object in the vector of scenes.
        RenderObject& object = first[i]; // Take the scene/object

        if (object.material != lastMaterial) { // Same material = (shaders/pipeline/descriptors) for multiple objects part of the same scene (e.g. monkey + triangles)
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
            uint32_t dynOffset = Helper::pad_uniform_buffer_size(*_device,sizeof(GPUSceneData)) * frameIndex;
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

void VulkanEngine::draw() { // todo : what need to be called at each frame? draw()? commandBuffers?
    // Wait GPU to render latest frame
    VK_CHECK(get_current_frame()._renderFence->wait(1000000000));  // wait until the GPU has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(get_current_frame()._renderFence->reset());

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_device->_logicalDevice, _swapchain->_swapchain, 1000000000, get_current_frame()._presentSemaphore->_semaphore, nullptr, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) { // || result == VK_SUBOPTIMAL_KHR
        recreate_swap_chain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        throw std::runtime_error("failed to acquire swap chain image");
    }

    render(imageIndex);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &(_swapchain->_swapchain);
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &(get_current_frame()._renderSemaphore->_semaphore);
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(_device->get_graphics_queue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreate_swap_chain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    _frameNumber++;
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

void VulkanEngine::ui_overlay(Statistics stats) {
    // Camera
    _camera->set_speed(_ui->get_settings().speed);
    _reset = _camera->update_camera(1. / stats.FrameRate);
    _camera->set_perspective(_ui->get_settings().fov, _ui->get_settings().aspect, _ui->get_settings().z_near,
                                 _ui->get_settings().z_far);

    // Scene
    _sceneParameters.sunlightColor = {_ui->get_settings().colors[0], _ui->get_settings().colors[1], _ui->get_settings().colors[2], 1.0};
    _sceneParameters.sunlightDirection = {_ui->get_settings().coordinates[0], _ui->get_settings().coordinates[1], _ui->get_settings().coordinates[2], _ui->get_settings().ambient};
    _sceneParameters.specularFactor = _ui->get_settings().specular;

    // Skybox
    _skyboxDisplay = _ui->p_open[SKYBOX_EDITOR];

    // Interface
    _ui->render(get_current_frame()._commandBuffer->_commandBuffer, stats);
}

void VulkanEngine::render(int imageIndex) {
    Statistics stats = monitoring();

    // --- Command Buffer
    VK_CHECK(vkResetCommandBuffer(get_current_frame()._commandBuffer->_commandBuffer, 0));

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(get_current_frame()._commandBuffer->_commandBuffer, &cmdBeginInfo));
    {
        // -- Clear value
        VkClearValue clearValue;
        clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        VkClearValue depthValue;
        depthValue.depthStencil.depth = 1.0f;
        std::array<VkClearValue, 2> clearValues = {clearValue, depthValue};

        // -- Render pass
        VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(_renderPass->_renderPass,
                                                                             _window->_windowExtent,
                                                                             _frameBuffers->_frameBuffers[imageIndex]);
        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = &clearValues[0];

        vkCmdBeginRenderPass(get_current_frame()._commandBuffer->_commandBuffer, &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);
        {

            // Scene
            if (_scene->_sceneIndex != _ui->get_settings().scene_index) {
                _scene->load_scene(_ui->get_settings().scene_index, *_camera);
                setup_descriptors();
            }
            // Draw
            draw_objects(get_current_frame()._commandBuffer->_commandBuffer, _scene->_renderables.data(),
                         _scene->_renderables.size());

            this->ui_overlay(stats);
        }
        vkCmdEndRenderPass(get_current_frame()._commandBuffer->_commandBuffer);

    }
    VK_CHECK(vkEndCommandBuffer(get_current_frame()._commandBuffer->_commandBuffer));

    // --- Submit queue
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &(get_current_frame()._presentSemaphore->_semaphore);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &(get_current_frame()._renderSemaphore->_semaphore);
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &get_current_frame()._commandBuffer->_commandBuffer;

    VK_CHECK(vkQueueSubmit(_device->get_graphics_queue(), 1, &submitInfo, get_current_frame()._renderFence->_fence));
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
        _skybox->destroy();
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
