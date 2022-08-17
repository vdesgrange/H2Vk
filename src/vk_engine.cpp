﻿#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vk_types.h>
#include <vk_initializers.h>
#include <iostream>

#include "vk_engine.h"
#include "VkBootstrap.h"


using namespace std;

#define VK_CHECK(x) \
    do \
    { \
        VkResult err = x; \
        if (err) { \
            std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort(); \
        } \
    } while (0)

void VulkanEngine::init_window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    _window = glfwCreateWindow(_windowExtent.width, _windowExtent.height, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}

void VulkanEngine::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void VulkanEngine::init()
{
	// We initialize SDL and create a window with it. 
	SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    init_window();
    init_vulkan();
    init_swapchain();
    init_commands();
    init_default_renderpass();
    init_framebuffers();
    init_sync_structures();
	
	//everything went fine
	_isInitialized = true;
}

void VulkanEngine::init_vulkan() {
//    create_instance();
//    setup_debug_messenger();
//
//    pickPhysicalDevice(deviceExtensions);
//    createLogicalDevice(deviceExtensions, validationLayers, enableValidationLayers);

    vkb::InstanceBuilder builder;

    //make the Vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
            .request_validation_layers(true)
            .require_api_version(1, 1, 0)
            .use_default_debug_messenger()
            .build();

    vkb::Instance vkb_inst = inst_ret.value();

    //store the instance
    _instance = vkb_inst.instance;
    //store the debug messenger
    _debug_messenger = vkb_inst.debug_messenger;

    // get the surface of the window we opened with SDL
    if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    //use vkbootstrap to select a GPU.
    //We want a GPU that can write to the SDL surface and supports Vulkan 1.1
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
            .set_minimum_version(1, 1)
            .set_surface(_surface)
            .select()
            .value();

    //create the final Vulkan device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a Vulkan application
    _device = vkbDevice.device;
    _physicalDevice = physicalDevice.physical_device;

    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::init_swapchain() {
    vkb::SwapchainBuilder swapchainBuilder{_physicalDevice, _device, _surface };

    vkb::Swapchain vkbSwapchain = swapchainBuilder
            .use_default_format_selection()
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(_windowExtent.width, _windowExtent.height)
            .build()
            .value();

    _swapchain = vkbSwapchain.swapchain;
    _swapChainImages = vkbSwapchain.get_images().value();
    _swapChainImageViews = vkbSwapchain.get_image_views().value();
    _swapChainImageFormat = vkbSwapchain.image_format;
}

void VulkanEngine::init_commands() {
    init_command_pool();
    init_command_buffer();
}

void VulkanEngine::init_command_pool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = _graphicsQueueFamily;
    poolInfo.pNext = nullptr;

    VK_CHECK(vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool));
}

void VulkanEngine::init_command_buffer() {
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = _commandPool;
    allocateInfo.commandBufferCount = 1; // why ? Shouldn't be a vector?
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.pNext = nullptr;

    VK_CHECK(vkAllocateCommandBuffers(_device, &allocateInfo, &_mainCommandBuffer));
}

void VulkanEngine::init_default_renderpass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VK_CHECK(vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass));
}

void VulkanEngine::init_framebuffers() {
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = _renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = _windowExtent.width;
    framebufferInfo.height = _windowExtent.height;
    framebufferInfo.layers = 1;

    _frameBuffers.resize(_swapChainImages.size());

    for (int i = 0; i < _swapChainImages.size(); i++) {
        framebufferInfo.pAttachments = &_swapChainImageViews[i];
        VK_CHECK(vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_frameBuffers[i]));
    }

}

void VulkanEngine::init_sync_structures() {
    // Used for CPU -> GPU communication
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    fenceInfo.pNext = nullptr;
    VK_CHECK(vkCreateFence(_device, &fenceInfo, nullptr, &_renderFence));

    //  Used for GPU -> GPU synchronisation
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;

    VK_CHECK(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderSemaphore));
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_presentSemaphore));
}

void VulkanEngine::cleanup()
{	
	if (_isInitialized) {
        for (int i = 0; i < _frameBuffers.size(); i++) {
            vkDestroyFramebuffer(_device, _frameBuffers[i], nullptr);
        }

        vkDestroyRenderPass(_device, _renderPass, nullptr);
        vkDestroyCommandPool(_device, _commandPool, nullptr);
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        for (int i = 0; i < _swapChainImageViews.size(); i++) {
            vkDestroyImageView(_device, _swapChainImageViews[i], nullptr);
        }

        vkDestroyDevice(_device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);

        glfwDestroyWindow(_window);
        glfwTerminate();
	}
}

void VulkanEngine::draw()
{
    // Wait GPU to render latest frame
    VK_CHECK(vkWaitForFences(_device, 1, &_renderFence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(_device, 1, &_renderFence));

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX,
                                            _presentSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        throw std::runtime_error("failed to acquire swap chain image");
    }

    VK_CHECK(vkResetCommandBuffer(_mainCommandBuffer, 0));
    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr; // VkCommandBufferInheritanceInfo for secondary cmd buff. defines states inheriting from primary cmd. buff.
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(_mainCommandBuffer, &cmdBeginInfo));

    VkClearValue clearValue; // screen clearing color
    float flash = abs(sin(_frameNumber / 120.f));
    clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.renderPass = _renderPass;
    renderPassInfo.framebuffer = _frameBuffers[imageIndex]; // Which image to render from the swapchain
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = _windowExtent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(_mainCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // todo

    vkCmdEndRenderPass(_mainCommandBuffer);
    VK_CHECK(vkEndCommandBuffer(_mainCommandBuffer));

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &_presentSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_renderSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_mainCommandBuffer;

    VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _renderFence));

    VkSwapchainKHR swapchains[] = {_swapchain};
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_renderSemaphore;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    _frameNumber += 1;
}

void VulkanEngine::run()
{
    while(!glfwWindowShouldClose(_window)) {
        glfwPollEvents();
        draw();
    }

    vkDeviceWaitIdle(_device);

//	SDL_Event e;
//	bool bQuit = false;
//
//	//main loop
//	while (!bQuit)
//	{
//		//Handle events on queue
//		while (SDL_PollEvent(&e) != 0)
//		{
//			//close the window when user alt-f4s or clicks the X button
//			if (e.type == SDL_QUIT) bQuit = true;
//		}
//
//		draw();
//	}
}

