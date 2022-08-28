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
#include "vk_camera.h"
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

    _window = glfwCreateWindow(CWIDTH, CHEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}

void VulkanEngine::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void VulkanEngine::init()
{
    init_window();
    init_camera();
    init_vulkan();
    init_swapchain();
    init_commands();
    init_default_renderpass();
    init_framebuffers();
    init_sync_structures();
    init_pipelines();
	load_meshes();
    init_scene();

	_isInitialized = true;
}

void VulkanEngine::init_vulkan() {
    _device = new Device(_window);
//    vkb::InstanceBuilder builder;
//
//    //make the Vulkan instance, with basic debug features
//    auto inst_ret = builder.set_app_name("Vulkan Application")
//            .request_validation_layers(true)
//            .require_api_version(1, 1, 0)
//            .use_default_debug_messenger()
//            .build();
//
//    vkb::Instance vkb_inst = inst_ret.value();
//
//    //store the instance
//    _instance = vkb_inst.instance;
//    //store the debug messenger
//    _debug_messenger = vkb_inst.debug_messenger;
//
//    // Get the surface of the window we opened with SDL
//    if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create window surface!");
//    }
//
//    // Select a GPU
//    //We want a GPU that can write to the SDL surface and supports Vulkan 1.1
//    vkb::PhysicalDeviceSelector selector{ vkb_inst };
//    vkb::PhysicalDevice physicalDevice = selector
//            .set_minimum_version(1, 1)
//            .set_surface(_surface)
//            .select()
//            .value();
//
//    //create the final Vulkan device
//    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
//    vkb::Device vkbDevice = deviceBuilder.build().value();
//
//    // Get the VkDevice handle used in the rest of a Vulkan application
//    _device = vkbDevice.device;
//    _physicalDevice = physicalDevice.physical_device;
//
//    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
//    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
//
//    // Initialize memory allocator
//    VmaAllocatorCreateInfo allocatorInfo = {};
//    allocatorInfo.physicalDevice = _physicalDevice;
//    allocatorInfo.device = _device;
//    allocatorInfo.instance = _instance;
//    vmaCreateAllocator(&allocatorInfo, &_allocator);
//
//    _mainDeletionQueue.push_function([=]() {
//        vmaDestroyAllocator(_allocator);
//    });
    return;
}

void VulkanEngine::init_scene() {
    RenderObject monkey;
    monkey.mesh = get_mesh("monkey");
    monkey.material = get_material("defaultMesh");
    monkey.transformMatrix = glm::mat4{ 1.0f };

    _renderables.push_back(monkey);

    for (int x = -20; x <= 20; x++) {
        for (int y = -20; y <= 20; y++) {
            RenderObject tri;
            tri.mesh = get_mesh("triangle");
            tri.material = get_material("defaultMesh");
            glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
            glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
            tri.transformMatrix = translation * scale;
            _renderables.push_back(tri);
        }
    }
}

void VulkanEngine::init_camera() {
    camera.set_flip_y(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);
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
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void VulkanEngine::init_swapchain() {
    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device->_physicalDevice, _device->_surface, &capabilities);

    VkExtent2D extent2d = choose_swap_extent(capabilities);
    _windowExtent = extent2d;

    vkb::SwapchainBuilder swapchainBuilder{_device->_physicalDevice, _device->_logicalDevice, _device->_surface };
    vkb::Swapchain vkbSwapchain = swapchainBuilder
            .use_default_format_selection()
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(_windowExtent.width, _windowExtent.height)
            .build()
            .value();

    _swapchain = vkbSwapchain.swapchain;
    _swapChainImages = vkbSwapchain.get_images().value();
    _swapChainImageViews = vkbSwapchain.get_image_views().value();
    _swapChainImageFormat = vkbSwapchain.image_format;

    VkExtent3D depthImageExtent = { // match window size
            _windowExtent.width,
            _windowExtent.height,
            1
    };
    _depthFormat = VK_FORMAT_D32_SFLOAT; // 32 bit

    VkImageCreateInfo imageInfo = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);
    VmaAllocationCreateInfo imageAllocInfo{};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(_device->_allocator, &imageInfo, &imageAllocInfo, &_depthImage._image, &_depthImage._allocation, nullptr);

    VkImageViewCreateInfo viewInfo = vkinit::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);
    VK_CHECK(vkCreateImageView(_device->_logicalDevice, &viewInfo, nullptr, &_depthImageView));

    // Clean up
    _swapChainDeletionQueue.push_function([=]() {
        vkDestroyImageView(_device->_logicalDevice, _depthImageView, nullptr);
        vmaDestroyImage(_device->_allocator, _depthImage._image, _depthImage._allocation);
        vkDestroySwapchainKHR(_device->_logicalDevice, _swapchain, nullptr);
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
    poolInfo.queueFamilyIndex = _device->get_graphics_queue_family();
    poolInfo.pNext = nullptr;

    VK_CHECK(vkCreateCommandPool(_device->_logicalDevice, &poolInfo, nullptr, &_commandPool));

    _mainDeletionQueue.push_function([=]() {
        vkDestroyCommandPool(_device->_logicalDevice, _commandPool, nullptr);
    });
}

void VulkanEngine::init_command_buffer() {
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = _commandPool;
    allocateInfo.commandBufferCount = 1; // why ? Shouldn't be a vector?
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.pNext = nullptr;

    VK_CHECK(vkAllocateCommandBuffers(_device->_logicalDevice, &allocateInfo, &_mainCommandBuffer));
}

void VulkanEngine::init_default_renderpass() {

    // === Color ===
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

    // === Depth ===
    // Must be hook to subpass
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = _depthFormat;
    depthAttachment.flags = 0;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubpassDependency depthDependency{};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.dstSubpass = 0;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask = 0;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    std::array<VkSubpassDependency, 2> dependencies = {dependency, depthDependency};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK(vkCreateRenderPass(_device->_logicalDevice, &renderPassInfo, nullptr, &_renderPass));

    _swapChainDeletionQueue.push_function([=]() {
        vkDestroyRenderPass(_device->_logicalDevice, _renderPass, nullptr);
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
        std::array<VkImageView, 2> attachments = {_swapChainImageViews[i], _depthImageView};
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();

        VK_CHECK(vkCreateFramebuffer(_device->_logicalDevice, &framebufferInfo, nullptr, &_frameBuffers[i]));
        _swapChainDeletionQueue.push_function([=]() {
            vkDestroyFramebuffer(_device->_logicalDevice, _frameBuffers[i], nullptr);
            vkDestroyImageView(_device->_logicalDevice, _swapChainImageViews[i], nullptr);
        });
    }
}

void VulkanEngine::init_sync_structures() {
    // Used for CPU -> GPU communication

    VkFenceCreateInfo fenceInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(_device->_logicalDevice, &fenceInfo, nullptr, &_renderFence));

    _mainDeletionQueue.push_function([=]() { // Destruction of fence
        vkDestroyFence(_device->_logicalDevice, _renderFence, nullptr);
    });

    //  Used for GPU -> GPU synchronisation
    VkSemaphoreCreateInfo semaphoreInfo = vkinit::semaphore_create_info();
    VK_CHECK(vkCreateSemaphore(_device->_logicalDevice, &semaphoreInfo, nullptr, &_renderSemaphore));
    VK_CHECK(vkCreateSemaphore(_device->_logicalDevice, &semaphoreInfo, nullptr, &_presentSemaphore));

    _mainDeletionQueue.push_function([=]() { // Destruction of semaphores
        vkDestroySemaphore(_device->_logicalDevice, _presentSemaphore, nullptr);
        vkDestroySemaphore(_device->_logicalDevice, _renderSemaphore, nullptr);
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
    VK_CHECK(vkCreatePipelineLayout(_device->_logicalDevice, &pipeline_layout_info, nullptr, &_pipelineLayout));

    // Configure graphics pipeline - build the stage-create-info for both vertex and fragment stages
    PipelineBuilder pipelineBuilder;
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));
    pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
    pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder._depthStencil = vkinit::pipeline_depth_stencil_state_create_info(true, true, VK_COMPARE_OP_LESS);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) _windowExtent.width;
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
    _graphicsPipeline = pipelineBuilder.build_pipeline(_device->_logicalDevice, _renderPass);

    // === 2 - Build red triangle pipeline ===
    pipelineBuilder._shaderStages.clear();
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, redTriangleVertexShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, redTriangleFragShader));
    _redTrianglePipeline = pipelineBuilder.build_pipeline(_device->_logicalDevice, _renderPass);

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
    VK_CHECK(vkCreatePipelineLayout(_device->_logicalDevice, &mesh_pipeline_layout_info, nullptr, &_meshPipelineLayout));

    pipelineBuilder._pipelineLayout = _meshPipelineLayout;
    _meshPipeline = pipelineBuilder.build_pipeline(_device->_logicalDevice, _renderPass);
    create_material(_meshPipeline, _meshPipelineLayout, "defaultMesh");

    // === 4 - Clean
    // Deleting shaders
    vkDestroyShaderModule(_device->_logicalDevice, redTriangleVertexShader, nullptr);
    vkDestroyShaderModule(_device->_logicalDevice, redTriangleFragShader, nullptr);
    vkDestroyShaderModule(_device->_logicalDevice, triangleFragShader, nullptr);
    vkDestroyShaderModule(_device->_logicalDevice, triangleVertexShader, nullptr);
    vkDestroyShaderModule(_device->_logicalDevice, meshVertShader, nullptr);

    _swapChainDeletionQueue.push_function([=]() {
        vkDestroyPipeline(_device->_logicalDevice, _redTrianglePipeline, nullptr);
        vkDestroyPipeline(_device->_logicalDevice, _graphicsPipeline, nullptr);
        vkDestroyPipeline(_device->_logicalDevice, _meshPipeline, nullptr);
        vkDestroyPipelineLayout(_device->_logicalDevice, _pipelineLayout, nullptr);
        vkDestroyPipelineLayout(_device->_logicalDevice, _meshPipelineLayout, nullptr);
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
    if (vkCreateShaderModule(_device->_logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
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

    _objMesh.load_from_obj("../assets/monkey_smooth.obj");

    upload_mesh(_mesh);
    upload_mesh(_objMesh);

    _meshes["monkey"] = _objMesh;
    _meshes["triangle"] = _mesh;
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
    VkResult result = vmaCreateBuffer(_device->_allocator, &bufferInfo, &vmaallocInfo,
                             &mesh._vertexBuffer._buffer,
                             &mesh._vertexBuffer._allocation,
                             nullptr);
    VK_CHECK(result);

    //add the destruction of triangle mesh buffer to the deletion queue
    _mainDeletionQueue.push_function([=]() {

        vmaDestroyBuffer(_device->_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation);
    });

    void* data;
    vmaMapMemory(_device->_allocator, mesh._vertexBuffer._allocation, &data);
    memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(_device->_allocator, mesh._vertexBuffer._allocation);
}

void VulkanEngine::recreate_swap_chain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(_device->_logicalDevice);
    _swapChainDeletionQueue.flush();

    init_swapchain();
    init_default_renderpass();
    init_framebuffers();
    init_pipelines();
}

void VulkanEngine::cleanup()
{	
	if (_isInitialized) {
        vkDeviceWaitIdle(_device->_logicalDevice);
        vkWaitForFences(_device->_logicalDevice, 1, &_renderFence, true, 1000000000);
        _swapChainDeletionQueue.flush();
        _mainDeletionQueue.flush();

        vkDestroyDevice(_device->_logicalDevice, nullptr);
        vkDestroySurfaceKHR(_device->_instance, _device->_surface, nullptr);
        vkb::destroy_debug_utils_messenger(_device->_instance, _device->_debug_messenger);
        vkDestroyInstance(_device->_instance, nullptr);

        glfwDestroyWindow(_window);
        glfwTerminate();
	}
}

void VulkanEngine::draw()
{
    // Wait GPU to render latest frame
    //wait until the GPU has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(_device->_logicalDevice, 1, &_renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(_device->_logicalDevice, 1, &_renderFence));

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_device->_logicalDevice, _swapchain, 1000000000, _presentSemaphore, nullptr, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) { // || result == VK_SUBOPTIMAL_KHR
        recreate_swap_chain();
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
    float flash = abs(sin(_frameNumber / 120.f));
    clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };
    VkClearValue depthValue;
    depthValue.depthStencil.depth = 1.0f;
    std::array<VkClearValue, 2> clearValues = {clearValue, depthValue};

    VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(_renderPass, _windowExtent, _frameBuffers[imageIndex]);
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = &clearValues[0];
    vkCmdBeginRenderPass(_mainCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

//    if(_selectedShader == 0) {
//        vkCmdBindPipeline(_mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
//    } else {
//        vkCmdBindPipeline(_mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _redTrianglePipeline);
//    }
//    vkCmdDraw(_mainCommandBuffer, 3, 1, 0, 0);

//    vkCmdBindPipeline(_mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);
//    VkDeviceSize offset = 0;
//    vkCmdBindVertexBuffers(_mainCommandBuffer, 0, 1, &_objMesh._vertexBuffer._buffer, &offset);
//
//    glm::vec3 camPos = { 0.f,0.f,-2.f };
//    glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
//    glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
//    projection[1][1] *= -1;
//    glm::mat4 model = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_frameNumber * 0.4f), glm::vec3(0, 1, 0));
//    glm::mat4 mesh_matrix = projection * view * model;
//    MeshPushConstants constants;
//    constants.render_matrix = mesh_matrix;
//
//    //upload the matrix to the GPU via push constants
//    vkCmdPushConstants(_mainCommandBuffer, _meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
//    vkCmdDraw(_mainCommandBuffer, _objMesh._vertices.size(), 1, 0, 0);

    draw_objects(_mainCommandBuffer, _renderables.data(), _renderables.size());
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

    VK_CHECK(vkQueueSubmit(_device->get_graphics_queue(), 1, &submitInfo, _renderFence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_renderSemaphore;
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

    vkDeviceWaitIdle(_device->_logicalDevice);
}

Material* VulkanEngine::create_material(VkPipeline pipeline, VkPipelineLayout pipelineLayout, const std::string &name) {
    Material mat;
    mat.pipeline = pipeline;
    mat.pipelineLayout = pipelineLayout;
    _materials[name] = mat;
    return &_materials[name];
}

Material* VulkanEngine::get_material(const std::string &name) {
    auto it = _materials.find(name);
    if ( it == _materials.end()) {
        return nullptr;
    } else {
        return &(*it).second;
    }
}

Mesh* VulkanEngine::get_mesh(const std::string &name) {
    auto it = _meshes.find(name);
    if ( it == _meshes.end()) {
        return nullptr;
    } else {
        return &(*it).second;
    }
}

void VulkanEngine::draw_objects(VkCommandBuffer commandBuffer, RenderObject *first, int count) {
    Mesh* lastMesh = nullptr;
    Material* lastMaterial = nullptr;

    for (int i=0; i < count; i++) {
        RenderObject& object = first[i];
        if (object.material != lastMaterial) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
        }

        glm::mat4 model = object.transformMatrix;
        glm::mat4 mesh_matrix = camera.get_mesh_matrix(model);

        MeshPushConstants constants;
        constants.render_matrix = mesh_matrix;

        vkCmdPushConstants(commandBuffer,
                           object.material->pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0,
                           static_cast<uint32_t>(sizeof(MeshPushConstants)),
                           &constants);

        if (object.mesh != lastMesh) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object.mesh->_vertexBuffer._buffer, &offset);
            lastMesh = object.mesh;
        }

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(object.mesh->_vertices.size()), 1, 0, 0);
    }
}