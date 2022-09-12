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
#include <iostream>

#include "VkBootstrap.h"
#include "vk_mem_alloc.h"
#include "vk_engine.h"
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
#include "vk_mesh.h"
#include "vk_material.h"
#include "vk_camera.h"
#include "vk_pipeline.h"
#include "vk_buffer.h"

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
    init_scene();

	_isInitialized = true;
}

void VulkanEngine::init_window() {
    _window = new Window();
}

void VulkanEngine::init_vulkan() {
    _device = new Device(*_window);
    std::cout << "GPU has a minimum buffer alignment = " << _device->_gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;
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
        _frames[i]._renderFence = new Fence(*_device);
        _frames[i]._renderSemaphore = new Semaphore(*_device);
        _frames[i]._presentSemaphore =  new Semaphore(*_device);

        _mainDeletionQueue.push_function([=]() { // Destruction of semaphores
            delete _frames[i]._presentSemaphore;
            delete _frames[i]._renderSemaphore;
            delete _frames[i]._renderFence;
        });

    }
}

void VulkanEngine::init_descriptors() {
    const size_t sceneParamBufferSize = FRAME_OVERLAP * pad_uniform_buffer_size(sizeof(GPUSceneData));
    _sceneParameterBuffer = Buffer::create_buffer(*_device, sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    std::vector<VkDescriptorPoolSize> sizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = 10;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes = sizes.data();

    vkCreateDescriptorPool(_device->_logicalDevice, &pool_info, nullptr, &_descriptorPool);

    VkDescriptorSetLayoutBinding camBufferBinding = vkinit::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    VkDescriptorSetLayoutBinding sceneBinding = vkinit::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    VkDescriptorSetLayoutBinding bindings[] = { camBufferBinding, sceneBinding };

    VkDescriptorSetLayoutCreateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.pNext = nullptr;
    setInfo.bindingCount = 2;
    setInfo.flags = 0;
    setInfo.pBindings = bindings;

    vkCreateDescriptorSetLayout(_device->_logicalDevice, &setInfo, nullptr, &_globalSetLayout);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        _frames[i].cameraBuffer = Buffer::create_buffer(*_device, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorSetAllocateInfo allocInfo ={};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = _descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &_globalSetLayout;

        vkAllocateDescriptorSets(_device->_logicalDevice, &allocInfo, &_frames[i].globalDescriptor);

        VkDescriptorBufferInfo camBInfo{};
        camBInfo.buffer = _frames[i].cameraBuffer._buffer;
        camBInfo.offset = 0;
        camBInfo.range = sizeof(GPUCameraData);

        VkDescriptorBufferInfo sceneBInfo{};
        sceneBInfo.buffer = _sceneParameterBuffer._buffer;
        // sceneBInfo.offset = pad_uniform_buffer_size(sizeof(GPUSceneData)) * i;
        // sceneBInfo.offset = 0;
        sceneBInfo.range = sizeof(GPUSceneData);

        VkWriteDescriptorSet camWrite = vkinit::write_descriptor_set(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _frames[i].globalDescriptor, &camBInfo, 0);
        VkWriteDescriptorSet sceneWrite = vkinit::write_descriptor_set(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, _frames[i].globalDescriptor, &sceneBInfo, 1);
        VkWriteDescriptorSet setWrites[] = {camWrite, sceneWrite};

        vkUpdateDescriptorSets(_device->_logicalDevice, 2, setWrites, 0, nullptr);
    }

    _mainDeletionQueue.push_function([&]() {
        vmaDestroyBuffer(_device->_allocator, _sceneParameterBuffer._buffer, _sceneParameterBuffer._allocation);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vmaDestroyBuffer(_device->_allocator, _frames[i].cameraBuffer._buffer, _frames[i].cameraBuffer._allocation);
        }
        vkDestroyDescriptorSetLayout(_device->_logicalDevice, _globalSetLayout, nullptr);
        vkDestroyDescriptorPool(_device->_logicalDevice, _descriptorPool, nullptr);
    });
}

FrameData& VulkanEngine::get_current_frame()
{
    return _frames[_frameNumber % FRAME_OVERLAP];
}

void VulkanEngine::init_pipelines() {
    _pipelineBuilder = new PipelineBuilder(*_window, *_device, *_renderPass, _globalSetLayout);
}

void VulkanEngine::load_meshes()
{
    _meshManager = new MeshManager(*_device);
}

void VulkanEngine::init_scene() {
    RenderObject monkey;
    monkey.mesh = _meshManager->get_mesh("monkey");
    monkey.material = _pipelineBuilder->get_material("defaultMesh");
    monkey.transformMatrix = glm::mat4{ 1.0f };

    _renderables.push_back(monkey);

    for (int x = -20; x <= 20; x++) {
        for (int y = -20; y <= 20; y++) {
            RenderObject tri;
            tri.mesh = _meshManager->get_mesh("triangle");
            tri.material = _pipelineBuilder->get_material("defaultMesh");
            glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
            glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
            tri.transformMatrix = translation * scale;
            _renderables.push_back(tri);
        }
    }
}

void VulkanEngine::recreate_swap_chain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(_window->_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window->_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(_device->_logicalDevice);
    _renderables.clear(); // Possible memory leak. Revoir comment gerer les scenes
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

size_t VulkanEngine::pad_uniform_buffer_size(size_t originalSize) {
    size_t minUboAlignment = _device->_gpuProperties.limits.minUniformBufferOffsetAlignment;
    size_t alignedSize = originalSize;
    if (minUboAlignment > 0 ) {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }

    return alignedSize;
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

    float framed = (_frameNumber / 120.f);
    _sceneParameters.ambientColor = { sin(framed),0,cos(framed),1 };
    char* sceneData;
    vmaMapMemory(_device->_allocator, _sceneParameterBuffer._allocation , (void**)&sceneData);
    int frameIndex = _frameNumber % FRAME_OVERLAP;
    sceneData += pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;
    memcpy(sceneData, &_sceneParameters, sizeof(GPUSceneData));
    vmaUnmapMemory(_device->_allocator, _sceneParameterBuffer._allocation);


    for (int i=0; i < count; i++) {
        RenderObject& object = first[i];
        if (object.material != lastMaterial) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
            uint32_t dynOffset = pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &get_current_frame().globalDescriptor, 1, &dynOffset);
        }

        //glm::mat4 model = object.transformMatrix;
        //glm::mat4 mesh_matrix = _camera->get_mesh_matrix(model);

        MeshPushConstants constants;
        // constants.render_matrix = mesh_matrix;
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

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(object.mesh->_vertices.size()), 1, 0, 0);
    }
}

void VulkanEngine::draw()
{
    // Wait GPU to render latest frame
    //wait until the GPU has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(get_current_frame()._renderFence->wait(1000000000));
    VK_CHECK(get_current_frame()._renderFence->reset());
//    VK_CHECK(_renderFence->wait(1000000000));
//    VK_CHECK(_renderFence->reset());

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_device->_logicalDevice, _swapchain->_swapchain, 1000000000, get_current_frame()._presentSemaphore->_semaphore, nullptr, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) { // || result == VK_SUBOPTIMAL_KHR
        recreate_swap_chain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        throw std::runtime_error("failed to acquire swap chain image");
    }

    VK_CHECK(vkResetCommandBuffer(get_current_frame()._commandBuffer->_mainCommandBuffer, 0));
    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr; // VkCommandBufferInheritanceInfo for secondary cmd buff. defines states inheriting from primary cmd. buff.
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(get_current_frame()._commandBuffer->_mainCommandBuffer, &cmdBeginInfo));

    VkClearValue clearValue;
    float flash = abs(sin(_frameNumber / 120.f));
    clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };
    VkClearValue depthValue;
    depthValue.depthStencil.depth = 1.0f;
    std::array<VkClearValue, 2> clearValues = {clearValue, depthValue};

    VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(_renderPass->_renderPass, _window->_windowExtent, _frameBuffers->_frameBuffers[imageIndex]);
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = &clearValues[0];
    vkCmdBeginRenderPass(get_current_frame()._commandBuffer->_mainCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    draw_objects(get_current_frame()._commandBuffer->_mainCommandBuffer, _renderables.data(), _renderables.size());
    vkCmdEndRenderPass(get_current_frame()._commandBuffer->_mainCommandBuffer);

    VK_CHECK(vkEndCommandBuffer(get_current_frame()._commandBuffer->_mainCommandBuffer));
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

        delete _meshManager;
        delete _pipelineBuilder;
        // Should I handle semaphores here? static queue (shared) maybe?
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
