/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif

#ifndef VMA_DEBUG_LOG
#define VMA_DEBUG_LOG
#endif

#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include <iostream>
#include <array>

#include "vk_engine.h"

/**
 * @brief Initialize the engine
 */
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
    init_materials();

    update_objects_buffer(_scene->_renderables.data(), _scene->_renderables.size());
	_isInitialized = true;
}

/**
 * @brief Initialize window surface (glfw)
 */
void VulkanEngine::init_window() {
    _window = std::make_unique<Window>();
}

/**
 * @brief Handle physical devices and their logical representation used by vulkan.
 */
void VulkanEngine::init_vulkan() {
    _device = std::make_unique<Device>(*_window);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        QueryTimestamp::init(*_device, &g_frames[i]._queryTimestamp, VK_QUERY_TYPE_TIMESTAMP, 4);
    }
}

/**
 * @brief Initialize swapchain
 * Array of presentable images that are associated with the surface.
 */
void VulkanEngine::init_swapchain() {
    _swapchain = std::make_unique<SwapChain>(*_window, *_device);
}

/**
 * @brief Initialize command pool and buffers for each frame
 * Set up upload context structure used for some pre-computation
 */
void VulkanEngine::init_commands() {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        g_frames[i]._commandPool = new CommandPool(*_device);
        g_frames[i]._commandBuffer = new CommandBuffer(*_device, *g_frames[i]._commandPool);
        _mainDeletionQueue.push_function([=]() {
            delete g_frames[i]._commandPool;
        });
    }

    _uploadContext._commandPool = new CommandPool(*_device);
    _uploadContext._commandBuffer = new CommandBuffer(*_device, *_uploadContext._commandPool);
}

/**
 * @brief Initialize render pass used for presentation
 * Collection of attachments, subpasses, dependencies between the subpasses, etc.
 */
void VulkanEngine::init_default_renderpass() {
    _renderPass = std::make_unique<RenderPass>(*_device);

    RenderPass::Attachment color = RenderPass::color(_swapchain->_swapChainImageFormat);
    RenderPass::Attachment depth = RenderPass::depth(_swapchain->_depthFormat);
    std::vector<VkAttachmentDescription> attachments = {color.description, depth.description};
    std::vector<VkSubpassDependency> dependencies = {color.dependency, depth.dependency};
    std::vector<VkAttachmentReference> references = {color.ref};
    std::vector<VkSubpassDescription> subpasses = {RenderPass::subpass_description(references, &depth.ref)};

    _renderPass->init(attachments, dependencies, subpasses);
}

/**
 * @brief Initialize framebuffers
 * A collection of memory attachments that a render pass instance uses.
 * Images used for the framebuffer attachment must be created for every images in the swap chain (ie. color + depth).
 */
void VulkanEngine::init_framebuffers() {
    _frameBuffers.clear();
    _frameBuffers.reserve(_swapchain->_swapChainImages.size());

    for (int i = 0; i < _swapchain->_swapChainImages.size(); i++) {
        std::vector<VkImageView> attachments;
        attachments.push_back(_swapchain->_swapChainImageViews[i]);
        attachments.push_back(_swapchain->_depthImageView);

        _frameBuffers.emplace_back(FrameBuffer(*_renderPass, attachments, _window->_windowExtent.width, _window->_windowExtent.height, 1));
    }
}

/**
 * @brief Initialize synchronisation structures used by frames (fence, semaphore).
 */
void VulkanEngine::init_sync_structures() {
    //  Used for GPU -> GPU synchronisation
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        g_frames[i]._renderFence = new Fence(*_device, VK_FENCE_CREATE_SIGNALED_BIT);
        g_frames[i]._renderSemaphore = new Semaphore(*_device);
        g_frames[i]._presentSemaphore =  new Semaphore(*_device);

        _mainDeletionQueue.push_function([=]() { // Destruction of semaphores
            delete g_frames[i]._presentSemaphore;
            delete g_frames[i]._renderSemaphore;
            delete g_frames[i]._renderFence;
        });
    }

    _uploadContext._uploadFence = new Fence(*_device);
    _mainDeletionQueue.push_function([=]() {
        delete _uploadContext._uploadFence;
    });
}

/**
 * @brief Initialize system managers
 * @note material, assets, lighting managers.
 */
void VulkanEngine::init_managers() {
    _pipelineBuilder = std::make_unique<GraphicPipeline>(*_device, *_renderPass);

    _systemManager = std::make_unique<SystemManager>();
    _materialManager = _systemManager->register_system<MaterialManager>(_device.get(), _pipelineBuilder.get());
    _meshManager = _systemManager->register_system<MeshManager>(_device.get(), &_uploadContext);
    _lightingManager = _systemManager->register_system<LightingManager>();
}

/**
 * @brief Initialize user interface
 * @note - todo - messy, need rework once ECS implemented
 */
void VulkanEngine::init_interface() {
    auto settings = Settings{.scene_index=0};
    _ui = std::make_unique<UInterface>(*this, settings);
    _ui->init_imgui();
}

/**
 * @brief Initialize camera
 * @note - todo - should consider entity-component system
 */
void VulkanEngine::init_camera() {
    _camera = std::make_unique<Camera>();
    _camera->inverse(false);
    _camera->set_position({ 0.f, 0.f, 0.f });
    _camera->set_perspective(70.f, (float)_window->_windowExtent.width /(float)_window->_windowExtent.height, 0.1f, 200.0f);

    _window->on_get_key = [this](int key, int action) {
        _camera->on_key(key, action);
//        if (updated) {
//            update_buffers();
//        }
    };

    _window->on_cursor_position = [this](double xpos, double ypos) {
        if (_ui->want_capture_mouse()) return;
        _camera->on_cursor_position(xpos, ypos);
//        if (updated) {
//            update_uniform_buffers();
//        }
    };

    _window->on_mouse_button = [this](int button, int action, int mods) {
        if (_ui->want_capture_mouse()) return;
        _camera->on_mouse_button(button, action, mods);
    };
}

/**
 * @brief Initialize the scene
 */
void VulkanEngine::init_scene() {
    _skybox = std::make_unique<Skybox>(*_device, *_meshManager, _uploadContext);
    _skybox->_type = Skybox::Type::box;
    _skybox->load();

    _cascadedShadow = std::make_unique<CascadedShadow>(*_device, _uploadContext);

    _atmosphere = std::make_unique<Atmosphere>(*_device, *_materialManager, *_lightingManager, _uploadContext);

    _sceneListing = std::make_unique<SceneListing>();
    _scene = std::make_unique<Scene>(*this);
}

/**
 * @brief Initialize descriptor objects
 * @note Initialize descriptors used by skybox, scene, shadow. Create caching and allocation system.
 */
void VulkanEngine::init_descriptors() {
    _layoutCache = new DescriptorLayoutCache(*_device); // todo : make smart pointer
    _allocator = new DescriptorAllocator(*_device); // todo : make smart pointer

    VulkanEngine::allocate_buffers(*_device);
    Camera::allocate_buffers(*_device);
    LightingManager::allocate_buffers(*_device);
    Scene::allocate_buffers(*_device);
    CascadedShadow::allocate_buffers(*_device);

    _skybox->setup_descriptors(*_layoutCache, *_allocator, _descriptorSetLayouts.skybox);
    _scene->setup_transformation_descriptors(*_layoutCache, *_allocator, _descriptorSetLayouts.matrices);
    _cascadedShadow->setup_descriptors(*_layoutCache, *_allocator, _descriptorSetLayouts.cascadedOffscreen);

    this->setup_environment_descriptors();

    // === Clean up === // Why keep this here if buffered allocated in their respective related class?
    _mainDeletionQueue.push_function([&]() {
        for (int i = 0; i < FRAME_OVERLAP; i++) {
            g_frames[i].cameraBuffer.destroy();
            g_frames[i].lightingBuffer.destroy();
            g_frames[i].objectBuffer.destroy();
            g_frames[i].cascadedOffscreenBuffer.destroy();
            g_frames[i].enabledFeaturesBuffer.destroy();
        }

        delete _layoutCache;
        delete _allocator;
    });
}

/**
 * @brief Initialize environment descriptors
 * @note Environment descriptor sets need to be initialized one time before run loop.
 */
void VulkanEngine::setup_environment_descriptors() {
    // Generic pool sizes : Try to maximize usage of a single Descriptor Pool by default.
    // Opposite: for optimality, just specify exact descriptor layout per shaders.
    std::vector<VkDescriptorPoolSize> poolSizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        g_frames[i].environmentDescriptor = VkDescriptorSet();

        VkDescriptorBufferInfo featuresBInfo{};
        featuresBInfo.buffer = g_frames[i].enabledFeaturesBuffer._buffer;
        featuresBInfo.offset = 0;
        featuresBInfo.range = sizeof(GPUEnabledFeaturesData);

        VkDescriptorBufferInfo camBInfo{};
        camBInfo.buffer = g_frames[i].cameraBuffer._buffer;
        camBInfo.offset = 0;
        camBInfo.range = sizeof(GPUCameraData);

        VkDescriptorBufferInfo lightingBInfo{};
        lightingBInfo.buffer = g_frames[i].lightingBuffer._buffer;
        lightingBInfo.range = sizeof(GPULightData);

       VkDescriptorBufferInfo offscreenBInfo{};
       offscreenBInfo.buffer = g_frames[i].cascadedOffscreenBuffer._buffer;
       offscreenBInfo.range = sizeof(CascadedShadow::GPUCascadedShadowData);

        DescriptorBuilder::begin(*_layoutCache, *_allocator)
                .bind_buffer(featuresBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                .bind_buffer(camBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1)
                .bind_buffer(lightingBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 2)
                .bind_buffer(offscreenBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 3)
                .bind_image(_skybox->_environment._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4) // precomputed no need to be binded every frame
                .bind_image(_skybox->_prefilter._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5) // precomputed no need to be binded every frame
                .bind_image(_skybox->_brdf._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6) // precomputed no need to be binded every frame
                .bind_image(_cascadedShadow->_depth._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  VK_SHADER_STAGE_FRAGMENT_BIT, 7)
                .layout(_descriptorSetLayouts.environment)
                .build(g_frames[i].environmentDescriptor, _descriptorSetLayouts.environment, poolSizes);

    }
}

/**
 * @brief Pre-compute materials
 */
void VulkanEngine::init_materials() {
    // === Skybox === (Build by default to handle if skybox enabled later)
    _skybox->setup_pipeline(*_materialManager, {_descriptorSetLayouts.skybox});
    _atmosphere->create_resources(*_layoutCache, *_allocator, *_renderPass);
    _atmosphere->precompute_resources();
}

/**
 * @brief Update user interface
 * Handle object which need update per frame (ie. camera movement)
 */
void VulkanEngine::ui_overlay() {
    Performance::Statistics stats = Performance::monitoring(_window.get(), _camera.get());
    stats._cmdTimestamps = get_current_frame()._queryTimestamp._results;

    bool updated = _ui->render(get_current_frame()._commandBuffer->_commandBuffer, stats);
    if (updated) {
        // Skybox
        _skybox->_display = _ui->p_open[SKYBOX_EDITOR];
    }

    // Move camera when key press
    _camera->update_camera(1.0f / stats.FrameRate);
}

/**
 * @brief build command buffer
 * @param frame
 * @param imageIndex
 */
void VulkanEngine::build_command_buffers(FrameData& frame, int imageIndex) {
    // Collection of attachments, subpasses, and dependencies between the subpasses
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(frame._commandBuffer->_commandBuffer, &cmdBeginInfo));

    frame._queryTimestamp.reset(frame._commandBuffer->_commandBuffer);

    // === Cascaded depth map render pass ===  
    if (_enabledFeatures.shadowMapping) {
        uint32_t start = frame._queryTimestamp.write(frame._commandBuffer->_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        _cascadedShadow->compute_resources(frame, _scene->_renderables);
        uint32_t end = frame._queryTimestamp.write(frame._commandBuffer->_commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        frame._queryTimestamp.record("Cascaded shadows", start, end);
    }

    // === Atmosphere skyview computing (WIP) ===
    if (_enabledFeatures.atmosphere) {
        _atmosphere->compute_resources(_frameNumber % FRAME_OVERLAP);
    }

    // === Scene render pass ===
    {
        // Record command buffers
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(_renderPass->_renderPass, _window->_windowExtent, _frameBuffers.at(imageIndex)._frameBuffer);
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        VkViewport viewport = vkinit::get_viewport((float) _window->_windowExtent.width, (float) _window->_windowExtent.height);
        vkCmdSetViewport(frame._commandBuffer->_commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = vkinit::get_scissor((float) _window->_windowExtent.width, (float) _window->_windowExtent.height);
        vkCmdSetScissor(frame._commandBuffer->_commandBuffer, 0, 1, &scissor);

        uint32_t start = frame._queryTimestamp.write(frame._commandBuffer->_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

        vkCmdBeginRenderPass(frame._commandBuffer->_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            // Debug shadow map
            if (_enabledFeatures.shadowMapping && this->_cascadedShadow->_debug) {
                this->_cascadedShadow->debug_depth(frame);
            } else {
                // === Skybox ===
                if (_enabledFeatures.skybox) {
                    this->_skybox->build_command_buffer(frame._commandBuffer->_commandBuffer, &frame.skyboxDescriptor);
                }

                // === Atmosphere (WIP) ===
                if (_enabledFeatures.atmosphere) {
                    this->_atmosphere->draw(frame._commandBuffer->_commandBuffer, &frame.atmosphereDescriptor);
                }

                // === Meshes ===
                if (_enabledFeatures.meshes) {
                   this->_scene->render_objects(frame._commandBuffer->_commandBuffer, frame);
                }
            }

            // === UI ===
            if (_enabledFeatures.ui) {
                this->ui_overlay();
            }

        }
        vkCmdEndRenderPass(frame._commandBuffer->_commandBuffer);
        uint32_t end = frame._queryTimestamp.write(frame._commandBuffer->_commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        frame._queryTimestamp.record("Full render", start, end);
    }
    VK_CHECK(vkEndCommandBuffer(frame._commandBuffer->_commandBuffer));

}

/**
 * @brief update objects buffer
 * @param first
 * @param count
 */
void VulkanEngine::update_objects_buffer(RenderObject *first, int count) {
    // Objects : write into the buffer by copying the render matrices from our render objects into it
    // Object not moving : call only when change scene
    for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
        FrameData& frame = g_frames[i]; // if object moves, call each frame.
        frame.objectBuffer.map();
        GPUObjectData* objectSSBO = (GPUObjectData*)frame.objectBuffer._data;
        for (int j = 0; j < count; j++) {
            RenderObject& object = first[j];
            objectSSBO[j].model = object.transformMatrix;
        }
        frame.objectBuffer.unmap();
    }
}

/**
 * @brief update uniform buffer
 * @note handle camera, lighting, shadow uniform buffer data.
 */
void VulkanEngine::update_uniform_buffers() {
    FrameData& frame =  get_current_frame();
    uint32_t frameIndex = _frameNumber % FRAME_OVERLAP;

    // === Camera & Objects & Environment ===
    // Camera : write into the buffer by copying the render matrices from camera object into it
    GPUCameraData camData = _camera->gpu_format();
    frame.cameraBuffer.map();
    frame.cameraBuffer.copyFrom(&camData, sizeof(GPUCameraData));
    frame.cameraBuffer.unmap();

    // Light : write scene data into lighting buffer
    GPULightData lightingData = _lightingManager->gpu_format();
    frame.lightingBuffer.map();
    frame.lightingBuffer.copyFrom(&lightingData, sizeof(GPULightData));
    frame.lightingBuffer.unmap();

    // Cascaded shadow
    CascadedShadow::GPUCascadedShadowData cascadedOffscreenData = _cascadedShadow->gpu_format();
    frame.cascadedOffscreenBuffer.map();
    frame.cascadedOffscreenBuffer.copyFrom(&cascadedOffscreenData, sizeof(CascadedShadow::GPUCascadedShadowData));
    frame.cascadedOffscreenBuffer.unmap();

    // === Enabled Features ===
    GPUEnabledFeaturesData featuresData{};
    featuresData.shadowMapping = _enabledFeatures.shadowMapping;
    featuresData.atmosphere = _enabledFeatures.atmosphere;
    featuresData.skybox = _enabledFeatures.skybox;

    frame.enabledFeaturesBuffer.map();
    frame.enabledFeaturesBuffer.copyFrom(&featuresData, sizeof(GPUEnabledFeaturesData));
    frame.enabledFeaturesBuffer.unmap();
}

/**
 * @brief Compute the next frame resources
 */
void VulkanEngine::compute() {
    // === Cascaded shadow mapping ===
    if (_enabledFeatures.shadowMapping) {
        _cascadedShadow->compute_cascades(*_camera, *_lightingManager);
    }

    // === Atmosphere skyview computing (WIP) ===
    if (_enabledFeatures.atmosphere) {
        _atmosphere->compute_resources(_frameNumber % FRAME_OVERLAP);
    }
}
 
/**
 * Perform computation for the next frame to be rendered
 * @brief Render the next frame
 * @param imageIndex current frame index
 */
void VulkanEngine::render(int imageIndex) {
    FrameData& frame = get_current_frame();

    // === Update scene ===
    if (_scene->_sceneIndex != _ui->get_settings().scene_index) {
        _scene->setup_texture_descriptors(*_layoutCache, *_allocator, _descriptorSetLayouts.textures);
        _scene->load_scene(_ui->get_settings().scene_index, *_camera);
        _cascadedShadow->setup_pipelines(*_device, *_materialManager, {_descriptorSetLayouts.cascadedOffscreen, _descriptorSetLayouts.matrices, _descriptorSetLayouts.textures}, *_renderPass);
        update_objects_buffer(_scene->_renderables.data(), _scene->_renderables.size());
    }

    // === Update resources ===
    compute();

    // === Update uniform buffers ===
    update_uniform_buffers();

    // === Render scene ===
    VK_CHECK(vkResetCommandBuffer(frame._commandBuffer->_commandBuffer, 0));

    build_command_buffers(frame, imageIndex);

    // === Submit queue ===
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.waitSemaphoreCount = 1; // Semaphore to wait before executing the command buffers
    submitInfo.pWaitSemaphores = &(frame._presentSemaphore->_semaphore); // Wait acquisition of next presentable image
    submitInfo.signalSemaphoreCount = 1; // Number of semaphores to be signaled once the commands are executed
    submitInfo.pSignalSemaphores = &(frame._renderSemaphore->_semaphore); // Signal image is rendered and ready to be presented
    submitInfo.commandBufferCount = 1; // Number of command buffers to execute in the batch
    submitInfo.pCommandBuffers = &frame._commandBuffer->_commandBuffer;

    VK_CHECK(vkQueueSubmit(_device->get_graphics_queue(), 1, &submitInfo, frame._renderFence->_fence));
}

/**
 * @brief Recreate swap-chain and affiliated objects.
 * @note Necessary for some action such as resizing the window.
 */
void VulkanEngine::recreate_swap_chain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(_window->_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window->_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(_device->_logicalDevice);
    _pipelineBuilder.reset();
    _renderPass.reset();
    _swapchain.reset();

    // Command buffers
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        delete g_frames[i]._commandBuffer;
        g_frames[i]._commandBuffer = new CommandBuffer(*_device, *g_frames[i]._commandPool);
    }
    delete _uploadContext._commandBuffer;
    _uploadContext._commandBuffer = new CommandBuffer(*_device, *_uploadContext._commandPool);

    init_swapchain();
    init_default_renderpass();
    init_framebuffers(); // framebuffers depend on renderpass for device for creation and destruction
    _pipelineBuilder = std::make_unique<GraphicPipeline>(*_device, *_renderPass); // todo: messy, rework
    _materialManager->_pipelineBuilder = _pipelineBuilder.get();

    if ((float)_window->_windowExtent.width > 0.0f && (float)_window->_windowExtent.height > 0.0f) {
        _camera->set_aspect((float)_window->_windowExtent.width / (float)_window->_windowExtent.height);
        update_uniform_buffers();
    }

}

/**
 * @brief Draw the latest rendered frame
 */
void VulkanEngine::draw() {
    FrameData& frame = get_current_frame();
    // === Prepare frame ===
    // Wait GPU to render latest frame. Timeout of 1 second
     VK_CHECK(frame._renderFence->wait(1000000000));
     VK_CHECK(frame._renderFence->reset());

    // Acquire next presentable image. Use occur only after the image is returned by vkAcquireNextImageKHR and before vkQueuePresentKHR.
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_device->_logicalDevice, _swapchain->_swapchain, 1000000000, frame._presentSemaphore->_semaphore, nullptr, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swap_chain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        throw std::runtime_error("failed to acquire swap chain image");
    }

    // === Render frame ===
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
    presentInfo.pWaitSemaphores = &(frame._renderSemaphore->_semaphore); // Wait for command buffers to be fully executed.

    result = vkQueuePresentKHR(_device->get_graphics_queue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _window->_framebufferResized) {
        _window->_framebufferResized = false;
        recreate_swap_chain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // === Time commands execution ===
    frame._queryTimestamp.results();

    _frameNumber++;
}

/**
 * @brief Main engine running loop
 */
void VulkanEngine::run()
{
    while(!glfwWindowShouldClose(_window->_window)) {
        glfwPollEvents();
        Window::glfw_get_key(_window->_window);
        draw();
    }

    vkDeviceWaitIdle(_device->_logicalDevice);
}

/**
 * @brief clean-up all resources before ending program
 */
void VulkanEngine::cleanup() {
    if (_isInitialized) {
        vkDeviceWaitIdle(_device->_logicalDevice);
        for (int i=0; i < FRAME_OVERLAP; i++) {
            g_frames[i]._renderFence->wait(1000000000);
            g_frames[i]._queryTimestamp.destroy();
        }

        _scene->_renderables.clear();
        _atmosphere.reset();
        _skybox.reset();
        _cascadedShadow.reset();
        _ui.reset();
        _meshManager.reset();
        _materialManager.reset();
        _systemManager.reset();
        _pipelineBuilder.reset();
        _frameBuffers.clear();
        _renderPass.reset();
        _swapchain.reset();
        _mainDeletionQueue.flush();

        delete _uploadContext._commandBuffer;
        delete _uploadContext._commandPool;

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
