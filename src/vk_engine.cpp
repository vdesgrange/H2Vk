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
#include "vk_pipeline.h"
#include "vk_initializers.h"


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

    _window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
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

    init_window();
    init_vulkan();
    init_swapchain();
    init_commands();
    init_default_renderpass();
    init_framebuffers();
    init_sync_structures();
    init_pipelines();
	load_meshes();

	_isInitialized = true;
}

void VulkanEngine::init_vulkan() {
    vkb::InstanceBuilder builder;

    //make the Vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Vulkan Application")
            .request_validation_layers(true)
            .require_api_version(1, 1, 0)
            .use_default_debug_messenger()
            .build();

    vkb::Instance vkb_inst = inst_ret.value();

    //store the instance
    _instance = vkb_inst.instance;
    //store the debug messenger
    _debug_messenger = vkb_inst.debug_messenger;

    // Get the surface of the window we opened with SDL
    if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    // Selecta GPU
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

    // Initialize memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _physicalDevice;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);

    _mainDeletionQueue.push_function([=]() {
        vmaDestroyAllocator(_allocator);
    });

}

VkExtent2D VulkanEngine::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.width);

        return actualExtent;
    }
}

void VulkanEngine::init_swapchain() {
    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &capabilities);
    VkExtent2D  extent2d = choose_swap_extent(capabilities);

    vkb::SwapchainBuilder swapchainBuilder{_physicalDevice, _device, _surface };
    vkb::Swapchain vkbSwapchain = swapchainBuilder
            .use_default_format_selection()
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(extent2d.width, extent2d.height)
            .build()
            .value();

    _swapchain = vkbSwapchain.swapchain;
    _swapChainImages = vkbSwapchain.get_images().value();
    _swapChainImageViews = vkbSwapchain.get_image_views().value();
    _swapChainImageFormat = vkbSwapchain.image_format;
    _swapChainDeletionQueue.push_function([=]() {
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    });
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

    _mainDeletionQueue.push_function([=]() {
        vkDestroyCommandPool(_device, _commandPool, nullptr);
    });
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

    _swapChainDeletionQueue.push_function([=]() {
        vkDestroyRenderPass(_device, _renderPass, nullptr);
    });
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
        _swapChainDeletionQueue.push_function([=]() {
            vkDestroyFramebuffer(_device, _frameBuffers[i], nullptr);
            vkDestroyImageView(_device, _swapChainImageViews[i], nullptr);
        });
    }
}

void VulkanEngine::init_sync_structures() {
    // Used for CPU -> GPU communication

    VkFenceCreateInfo fenceInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(_device, &fenceInfo, nullptr, &_renderFence));

    _mainDeletionQueue.push_function([=]() { // Destruction of fence
        vkDestroyFence(_device, _renderFence, nullptr);
    });

    //  Used for GPU -> GPU synchronisation
    VkSemaphoreCreateInfo semaphoreInfo = vkinit::semaphore_create_info();
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderSemaphore));
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_presentSemaphore));

    _mainDeletionQueue.push_function([=]() { // Destruction of semaphores
        vkDestroySemaphore(_device, _presentSemaphore, nullptr);
        vkDestroySemaphore(_device, _renderSemaphore, nullptr);
    });
}

void VulkanEngine::init_pipelines() {

    // Load shaders
    VkShaderModule triangleFragShader;
    if (!load_shader_module("../shaders/shader_base.frag.spv", &triangleFragShader))
    {
        std::cout << "Error when building the triangle fragment shader module" << std::endl;
    }
    else {
        std::cout << "Triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule triangleVertexShader;
    if (!load_shader_module("../shaders/shader_base.vert.spv", &triangleVertexShader))
    {
        std::cout << "Error when building the triangle vertex shader module" << std::endl;

    }
    else {
        std::cout << "Triangle vertex shader successfully loaded" << std::endl;
    }

    // Load shaders
    VkShaderModule redTriangleFragShader;
    if (!load_shader_module("../shaders/red_shader_base.frag.spv", &redTriangleFragShader))
    {
        std::cout << "Error when building the triangle fragment shader module" << std::endl;
    }
    else {
        std::cout << "Red triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule redTriangleVertexShader;
    if (!load_shader_module("../shaders/red_shader_base.vert.spv", &redTriangleVertexShader))
    {
        std::cout << "Error when building the triangle vertex shader module" << std::endl;

    }
    else {
        std::cout << "Red triangle vertex shader successfully loaded" << std::endl;
    }

    VkShaderModule meshVertShader;
    if (!load_shader_module("../shaders/tri_mesh.vert.spv", &meshVertShader)) {
        std::cout << "Error when building the green triangle mesh vertex shader module" << std::endl;
    }
    else {
        std::cout << "Green triangle vertex shader successfully loaded" << std::endl;
    }


    // Build pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_pipelineLayout));

    // Configure graphics pipeline - build the stage-create-info for both vertex and fragment stages
    PipelineBuilder pipelineBuilder;
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));
    pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
    pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)_windowExtent.width;
    viewport.height = (float)_windowExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    pipelineBuilder._viewport = viewport;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _windowExtent;
    pipelineBuilder._scissor = scissor;

    pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
    pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();
    pipelineBuilder._pipelineLayout = _pipelineLayout;

    // === 1 - Build graphics pipeline ===
    _graphicsPipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

    // === 2 - Build red triangle pipeline ===
    pipelineBuilder._shaderStages.clear();
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, redTriangleVertexShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, redTriangleFragShader));
    _redTrianglePipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

    // === 3 - Build dynamic triangle mesh
    pipelineBuilder._shaderStages.clear();  // Clear the shader stages for the builder

    VertexInputDescription vertexDescription = Vertex::get_vertex_description();

    // Connect the pipeline builder vertex input info to the one we get from Vertex
    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = static_cast<uint32_t>(sizeof(MeshPushConstants));
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipeline_layout_create_info();
    mesh_pipeline_layout_info.pPushConstantRanges = &push_constant;
    mesh_pipeline_layout_info.pushConstantRangeCount = 1;
    VK_CHECK(vkCreatePipelineLayout(_device, &mesh_pipeline_layout_info, nullptr, &_meshPipelineLayout));

    pipelineBuilder._pipelineLayout = _meshPipelineLayout;
    _meshPipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

    // === 4 - Clean
    // Deleting shaders
    vkDestroyShaderModule(_device, redTriangleVertexShader, nullptr);
    vkDestroyShaderModule(_device, redTriangleFragShader, nullptr);
    vkDestroyShaderModule(_device, triangleFragShader, nullptr);
    vkDestroyShaderModule(_device, triangleVertexShader, nullptr);
    vkDestroyShaderModule(_device, meshVertShader, nullptr);

    _mainDeletionQueue.push_function([=]() {
        vkDestroyPipeline(_device, _redTrianglePipeline, nullptr);
        vkDestroyPipeline(_device, _graphicsPipeline, nullptr);
        vkDestroyPipeline(_device, _meshPipeline, nullptr);
        vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
        vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr);
    });
}

bool VulkanEngine::load_shader_module(const char* filePath, VkShaderModule* out) {
    const std::vector<uint32_t> code = read_file(filePath);
    return create_shader_module(code, out);
}

std::vector<uint32_t> VulkanEngine::read_file(const char* filePath) {
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file.");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    return buffer;
}

bool VulkanEngine::create_shader_module(const std::vector<uint32_t>& code, VkShaderModule* out) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }

    *out = shaderModule;
    return true;
}

void VulkanEngine::load_meshes()
{
    _mesh._vertices.resize(3);

    _mesh._vertices[0].position = { 1.f, 1.f, 0.0f };
    _mesh._vertices[1].position = {-1.f, 1.f, 0.0f };
    _mesh._vertices[2].position = { 0.f,-1.f, 0.0f };

    _mesh._vertices[0].color = { 0.f, 1.f, 0.0f }; //pure green
    _mesh._vertices[1].color = { 0.f, 1.f, 0.0f }; //pure green
    _mesh._vertices[2].color = { 0.f, 1.f, 0.0f }; //pure green

    upload_mesh(_mesh);
}

void VulkanEngine::upload_mesh(Mesh& mesh)
{
    //allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    //this is the total size, in bytes, of the buffer we are allocating
    bufferInfo.size = mesh._vertices.size() * sizeof(Vertex);
    //this buffer is going to be used as a Vertex Buffer
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    //let the VMA library know that this data should be writeable by CPU, but also readable by GPU
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaallocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    //allocate the buffer
    VkResult result = vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
                             &mesh._vertexBuffer._buffer,
                             &mesh._vertexBuffer._allocation,
                             nullptr);
    VK_CHECK(result);

    //add the destruction of triangle mesh buffer to the deletion queue
    _mainDeletionQueue.push_function([=]() {

        vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation);
    });

    void* data;
    vmaMapMemory(_allocator, mesh._vertexBuffer._allocation, &data);
    memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(_allocator, mesh._vertexBuffer._allocation);
}

void VulkanEngine::recreate_swap_chain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(_device);
    _swapChainDeletionQueue.flush();

    init_swapchain();
    init_default_renderpass();
    init_framebuffers();
    init_pipelines();
}

void VulkanEngine::cleanup()
{	
	if (_isInitialized) {
        vkDeviceWaitIdle(_device);
        vkWaitForFences(_device, 1, &_renderFence, true, 1000000000);
        _swapChainDeletionQueue.flush();
        _mainDeletionQueue.flush();

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
    //wait until the GPU has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &_renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(_device, 1, &_renderFence));

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _presentSemaphore, nullptr, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) { // || result == VK_SUBOPTIMAL_KHR
        //recreate_swap_chain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        throw std::runtime_error("failed to acquire swap chain image");
    }


    VK_CHECK(vkResetCommandBuffer(_mainCommandBuffer, 0));
    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr; // VkCommandBufferInheritanceInfo for secondary cmd buff. defines states inheriting from primary cmd. buff.
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(_mainCommandBuffer, &cmdBeginInfo));

    VkClearValue clearValue;
    clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(_renderPass, _windowExtent, _frameBuffers[imageIndex]);
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;
    vkCmdBeginRenderPass(_mainCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

//    if(_selectedShader == 0) {
//        vkCmdBindPipeline(_mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
//    } else {
//        vkCmdBindPipeline(_mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _redTrianglePipeline);
//    }
//    vkCmdDraw(_mainCommandBuffer, 3, 1, 0, 0);

    vkCmdBindPipeline(_mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(_mainCommandBuffer, 0, 1, &_mesh._vertexBuffer._buffer, &offset);

    glm::vec3 camPos = { 0.f,0.f,-2.f };
    glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
    glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;
    glm::mat4 model = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_frameNumber * 0.4f), glm::vec3(0, 1, 0));
    glm::mat4 mesh_matrix = projection * view * model;
    MeshPushConstants constants;
    constants.render_matrix = mesh_matrix;

    //upload the matrix to the GPU via push constants
    vkCmdPushConstants(_mainCommandBuffer, _meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
    vkCmdDraw(_mainCommandBuffer, _mesh._vertices.size(), 1, 0, 0);
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

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_renderSemaphore;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        //recreate_swap_chain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    _frameNumber++;
}

void VulkanEngine::run()
{

    while(!glfwWindowShouldClose(_window)) {
        glfwPollEvents();

        if (glfwGetKey(_window, GLFW_KEY_DOWN) == GLFW_PRESS) {
             if (glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                _selectedShader += 1;
                if(_selectedShader > 1)
                {
                    _selectedShader = 0;
                }
            }
        }

        draw();
    }

    vkDeviceWaitIdle(_device);
}
