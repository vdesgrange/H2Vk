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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "VkBootstrap.h"
#include "vk_mem_alloc.h"
#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_window.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_command_pool.h"
#include "vk_command_buffer.h"
#include "vk_renderpass.h"
#include "vk_framebuffers.h"
#include "vk_fence.h"
#include "vk_mesh_manager.h"
#include "vk_semaphore.h"
#include "assets/vk_mesh.h"
#include "vk_material.h"
#include "vk_camera.h"
#include "vk_pipeline.h"
#include "vk_buffer.h"
#include "vk_texture.h"
#include "vk_descriptor_builder.h"
#include "vk_descriptor_cache.h"
#include "vk_descriptor_allocator.h"
#include "vk_imgui.h"
#include "vk_scene_listing.h"
#include "vk_scene.h"

#include "imgui.h"

using namespace std;

void VulkanEngine::init()
{
    init_window();
    init_vulkan();
    init_camera();
    init_swapchain();
    init_commands();
    init_default_renderpass();
    init_framebuffers();
    init_sync_structures();
    init_descriptors();
    init_pipelines();
	load_meshes();
    load_images();
    init_scene();
    init_interface();

	_isInitialized = true;
}

void VulkanEngine::init_window() {
    _window = new Window();
}

void VulkanEngine::init_vulkan() {
    _device = new Device(*_window);
    std::cout << "GPU has a minimum buffer alignment = " << _device->_gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;
}

void VulkanEngine::init_interface() {
    Settings settings = Settings{
            .p_open=true,
            .scene_index=0
    };

    _ui = new UInterface(*this, settings);
    _ui->init_imgui();
}

void VulkanEngine::init_camera() {
    _camera = new Camera{};
    _camera->set_flip_y(true);
    _camera->set_position({ 0.f, -6.f, -10.f });
    _camera->set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);
}

void VulkanEngine::init_swapchain() {
    _swapchain = new SwapChain(*_window, *_device);
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
    _renderPass = new RenderPass(*_device, *_swapchain);
}

void VulkanEngine::init_framebuffers() {
    _frameBuffers = new FrameBuffers(*_window, *_device, *_swapchain, *_renderPass);
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
    _layoutCache = new DescriptorLayoutCache(*_device);
    _allocator = new DescriptorAllocator(*_device);

    const size_t sceneParamBufferSize = FRAME_OVERLAP * Helper::pad_uniform_buffer_size(*_device, sizeof(GPUSceneData));
    _sceneParameterBuffer = Buffer::create_buffer(*_device, sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    VkDescriptorBufferInfo sceneBInfo{};
    sceneBInfo.buffer = _sceneParameterBuffer._buffer;
    sceneBInfo.range = sizeof(GPUSceneData);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        _frames[i].globalDescriptor = VkDescriptorSet();
        _frames[i].objectDescriptor = VkDescriptorSet();

        const uint32_t MAX_OBJECTS = 10000;
        _frames[i].cameraBuffer = Buffer::create_buffer(*_device, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        _frames[i].objectBuffer = Buffer::create_buffer(*_device, sizeof(GPUCameraData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo camBInfo{};
        camBInfo.buffer = _frames[i].cameraBuffer._buffer;
        camBInfo.offset = 0;
        camBInfo.range = sizeof(GPUCameraData);

        VkDescriptorBufferInfo objectsBInfo{};
        objectsBInfo.buffer = _frames[i].objectBuffer._buffer;
        objectsBInfo.offset = 0;
        objectsBInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

        DescriptorBuilder::begin(*_layoutCache, *_allocator)
                .bind_buffer(camBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
                .bind_buffer(sceneBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                .build(_frames[i].globalDescriptor, _globalSetLayout, poolSize.sizes);

        DescriptorBuilder::begin(*_layoutCache, *_allocator)
            .bind_buffer(objectsBInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
            .build(_frames[i].objectDescriptor, _objectSetLayout, poolSize.sizes);
    }

    DescriptorBuilder::begin(*_layoutCache, *_allocator)
            .bind_none(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
            .layout(_singleTextureSetLayout);

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

FrameData& VulkanEngine::get_current_frame()
{
    return _frames[_frameNumber % FRAME_OVERLAP];
}

void VulkanEngine::init_pipelines() {
    // VkDescriptorSetLayout setLayouts[] = {_globalSetLayout, _objectSetLayout, _singleTextureSetLayout};
    std::vector<VkDescriptorSetLayout> setLayouts = {_globalSetLayout, _objectSetLayout, _singleTextureSetLayout};
    _pipelineBuilder = new PipelineBuilder(*_window, *_device, *_renderPass, setLayouts);

    // move to scene listing as well? Only usage
}

void VulkanEngine::load_meshes() {
    _meshManager = new MeshManager(*_device, _uploadContext); // move to sceneListing ?
}

void VulkanEngine::load_images() {
    _textureManager = new TextureManager(*this);
    _samplerManager = new SamplerManager(*this);
}

void VulkanEngine::init_scene() {
    _sceneListing = new SceneListing(_meshManager, _textureManager, _pipelineBuilder);
    _scene = new Scene(*_meshManager, *_textureManager, *_pipelineBuilder);

    // If texture not loaded here, it creates issues -> Must be loaded before binding (previously in load_images)
    _textureManager->load_texture("../assets/lost_empire-RGBA.png", "empire_diffuse");

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);
    VkSampler blockySampler; // add cache for sampler
    vkCreateSampler(_device->_logicalDevice, &samplerInfo, nullptr, &blockySampler);
    _samplerManager->_loadedSampler["blocky_sampler"] = blockySampler;

    Material* texturedMat =	_pipelineBuilder->get_material("texturedMesh");

    VkDescriptorImageInfo imageBufferInfo;
    imageBufferInfo.sampler = _samplerManager->_loadedSampler["blocky_sampler"];
    imageBufferInfo.imageView = _textureManager->_loadedTextures["empire_diffuse"].imageView;
    imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    DescriptorBuilder::begin(*_layoutCache, *_allocator)
            .bind_image(imageBufferInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texturedMat->textureSet, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
            .build(texturedMat->textureSet, _singleTextureSetLayout, poolSize.sizes);

}

void VulkanEngine::recreate_swap_chain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(_window->_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window->_window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(_device->_logicalDevice);

    _scene->_renderables.clear(); // Possible memory leak. Revoir comment gerer les scenes
    delete _pipelineBuilder; // Revoir comment gerer pipeline avec scene.
    delete _frameBuffers;
    delete _renderPass;
    delete _swapchain;

    init_swapchain();
    init_default_renderpass();
    init_framebuffers();
    init_pipelines();
    init_scene();
}

void VulkanEngine::draw_objects(VkCommandBuffer commandBuffer, RenderObject *first, int count) {
    Mesh* lastMesh = nullptr;
    Material* lastMaterial = nullptr;

    GPUCameraData camData;
    camData.proj = _camera->perspective;
    camData.view = _camera->view;
    camData.viewproj = _camera->perspective * _camera->view;

    void* data;
    vmaMapMemory(_device->_allocator, get_current_frame().cameraBuffer._allocation, &data);
    memcpy(data, &camData, sizeof(GPUCameraData));
    vmaUnmapMemory(_device->_allocator, get_current_frame().cameraBuffer._allocation);

    void* objectData;
    vmaMapMemory(_device->_allocator, get_current_frame().objectBuffer._allocation, &objectData);
    GPUObjectData* objectSSBO = (GPUObjectData*)objectData;
    for (int i = 0; i < count; i++) {
        RenderObject& object = first[i];
        objectSSBO[i].modelMatrix = object.transformMatrix;
    }
    vmaUnmapMemory(_device->_allocator, get_current_frame().objectBuffer._allocation);

    float framed = (_frameNumber / 120.f);
    _sceneParameters.ambientColor = { sin(framed),0,cos(framed),1 };
    char* sceneData;
    vmaMapMemory(_device->_allocator, _sceneParameterBuffer._allocation , (void**)&sceneData);
    int frameIndex = _frameNumber % FRAME_OVERLAP;
    sceneData += Helper::pad_uniform_buffer_size(*_device,sizeof(GPUSceneData)) * frameIndex;
    memcpy(sceneData, &_sceneParameters, sizeof(GPUSceneData));
    vmaUnmapMemory(_device->_allocator, _sceneParameterBuffer._allocation);

    for (int i=0; i < count; i++) {
        RenderObject& object = first[i];
        if (object.material != lastMaterial) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
            uint32_t dynOffset = Helper::pad_uniform_buffer_size(*_device,sizeof(GPUSceneData)) * frameIndex;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &get_current_frame().globalDescriptor, 1, &dynOffset);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1, &get_current_frame().objectDescriptor, 0,nullptr);

            if (object.material->textureSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 2, 1, &object.material->textureSet, 0, nullptr);
            }
        }

        MeshPushConstants constants;
        constants.render_matrix = object.transformMatrix;

        vkCmdPushConstants(commandBuffer,
                           object.material->pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0,
                           sizeof(MeshPushConstants),
                           &constants);

        if (object.mesh != lastMesh) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object.mesh->_vertexBuffer._buffer, &offset);
            lastMesh = object.mesh;
        }

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(object.mesh->_vertices.size()), 1, 0, i);
    }
}

void VulkanEngine::draw() {
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

    render(imageIndex); // draw_data,

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

    return stats;
}

void VulkanEngine::render(int imageIndex) { // ImDrawData* draw_data,
    // --- Command Buffer
    VK_CHECK(vkResetCommandBuffer(get_current_frame()._commandBuffer->_mainCommandBuffer, 0));

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr; // VkCommandBufferInheritanceInfo for secondary cmd buff. defines states inheriting from primary cmd. buff.
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(get_current_frame()._commandBuffer->_mainCommandBuffer, &cmdBeginInfo));

    // -- Clear value
    VkClearValue clearValue;
//    float flash = abs(sin(_frameNumber / 120.f));
//    clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };
    clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    VkClearValue depthValue;
    depthValue.depthStencil.depth = 1.0f;
    std::array<VkClearValue, 2> clearValues = {clearValue, depthValue};

    // -- Render pass
    VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(_renderPass->_renderPass, _window->_windowExtent, _frameBuffers->_frameBuffers[imageIndex]);
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = &clearValues[0];
    vkCmdBeginRenderPass(get_current_frame()._commandBuffer->_mainCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Camera
    _camera->set_perspective(_ui->get_settings().fov, _ui->get_settings().aspect, _ui->get_settings().z_near, _ui->get_settings().z_far);
    _camera->set_position({_ui->get_settings().coordinates[0], _ui->get_settings().coordinates[1], _ui->get_settings().coordinates[2]});

    // Load scene (if new)
    if (_scene->_sceneIndex != _ui->get_settings().scene_index) {
        _scene->load_scene(_ui->get_settings().scene_index, *_camera);
        _renderables = _scene->_renderables;
    }

    draw_objects(get_current_frame()._commandBuffer->_mainCommandBuffer, _scene->_renderables.data(), _scene->_renderables.size());

    Statistics stats = monitoring();
    _ui->render(get_current_frame()._commandBuffer->_mainCommandBuffer, stats);

    vkCmdEndRenderPass(get_current_frame()._commandBuffer->_mainCommandBuffer);
    VK_CHECK(vkEndCommandBuffer(get_current_frame()._commandBuffer->_mainCommandBuffer));

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
    submitInfo.pCommandBuffers = &get_current_frame()._commandBuffer->_mainCommandBuffer;

    VK_CHECK(vkQueueSubmit(_device->get_graphics_queue(), 1, &submitInfo, get_current_frame()._renderFence->_fence));
}

void VulkanEngine::run()
{
    while(!glfwWindowShouldClose(_window->_window)) {
        glfwPollEvents();

        if (glfwGetKey(_window->_window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            if (glfwGetKey(_window->_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                _selectedShader += 1;
                if(_selectedShader > 1)
                {
                    _selectedShader = 0;
                }
            }
        }

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

        delete _ui;
        delete _samplerManager;
        delete _textureManager;
        delete _meshManager;
        delete _pipelineBuilder;
        delete _frameBuffers;
        delete _renderPass;
        delete _swapchain;
        _mainDeletionQueue.flush();

        // todo find a way to move this into vk_device without breaking swapchain
        vmaDestroyAllocator(_device->_allocator);
        vkDestroyDevice(_device->_logicalDevice, nullptr);
        vkDestroySurfaceKHR(_device->_instance, _device->_surface, nullptr);
        vkb::destroy_debug_utils_messenger(_device->_instance, _device->_debug_messenger);
        vkDestroyInstance(_device->_instance, nullptr);

        delete _device;
        delete _window;
        glfwTerminate();
    }
}
