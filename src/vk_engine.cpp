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
#include "vk_helpers.h"
#include "vk_camera.h"
#include "vk_mem_alloc.h"
#include "vk_engine.h"
#include "vk_pipeline.h"
#include "vk_initializers.h"

using namespace std;

void VulkanEngine::init_window() {
    _window = new Window();
//    glfwInit();
//    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
//
//    _window = glfwCreateWindow(CWIDTH, CHEIGHT, "Vulkan", nullptr, nullptr);
//    glfwSetWindowUserPointer(_window, this);
//    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
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
    _device = new Device(*_window, _mainDeletionQueue);
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

void VulkanEngine::init_swapchain() {
    _swapchain = new SwapChain(*_window, *_device);
}

void VulkanEngine::init_commands() {
    _commandPool = new CommandPool(*_device, _mainDeletionQueue);
    _commandBuffer = new CommandBuffer(*_device, *_commandPool);
}

void VulkanEngine::init_default_renderpass() {
    _renderPass = new RenderPass(*_device, *_swapchain);
}

void VulkanEngine::init_framebuffers() {
    _frameBuffers = new FrameBuffers(*_window, *_device, *_swapchain, *_renderPass);
}

void VulkanEngine::init_sync_structures() {
    _renderFence = new Fence(*_device);

    //  Used for GPU -> GPU synchronisation
    _renderSemaphore = new Semaphore(*_device);
    _presentSemaphore = new Semaphore(*_device);
//
//    VkSemaphoreCreateInfo semaphoreInfo = vkinit::semaphore_create_info();
//    VK_CHECK(vkCreateSemaphore(_device->_logicalDevice, &semaphoreInfo, nullptr, &_renderSemaphore));
//    VK_CHECK(vkCreateSemaphore(_device->_logicalDevice, &semaphoreInfo, nullptr, &_presentSemaphore));

    _mainDeletionQueue.push_function([=]() { // Destruction of semaphores
        delete _presentSemaphore;
        delete _renderSemaphore;
//        vkDestroySemaphore(_device->_logicalDevice, _presentSemaphore, nullptr);
//        vkDestroySemaphore(_device->_logicalDevice, _renderSemaphore, nullptr);
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
    viewport.width = (float) _window->_windowExtent.width;
    viewport.height = (float)_window->_windowExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    pipelineBuilder._viewport = viewport;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _window->_windowExtent;
    pipelineBuilder._scissor = scissor;

    pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
    pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();
    pipelineBuilder._pipelineLayout = _pipelineLayout;

    // === 1 - Build graphics pipeline ===
    _graphicsPipeline = pipelineBuilder.build_pipeline(*_device, *_renderPass);

    // === 2 - Build red triangle pipeline ===
    pipelineBuilder._shaderStages.clear();
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, redTriangleVertexShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, redTriangleFragShader));
    _redTrianglePipeline = pipelineBuilder.build_pipeline(*_device, *_renderPass);

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
    _meshPipeline = pipelineBuilder.build_pipeline(*_device, *_renderPass);
    create_material(_meshPipeline, _meshPipelineLayout, "defaultMesh");

    // === 4 - Clean
    // Deleting shaders
    vkDestroyShaderModule(_device->_logicalDevice, redTriangleVertexShader, nullptr);
    vkDestroyShaderModule(_device->_logicalDevice, redTriangleFragShader, nullptr);
    vkDestroyShaderModule(_device->_logicalDevice, triangleFragShader, nullptr);
    vkDestroyShaderModule(_device->_logicalDevice, triangleVertexShader, nullptr);
    vkDestroyShaderModule(_device->_logicalDevice, meshVertShader, nullptr);

    _swapchain->_swapChainDeletionQueue.push_function([=]() {
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
    glfwGetFramebufferSize(_window->_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window->_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(_device->_logicalDevice);
    _swapchain->_swapChainDeletionQueue.flush();

    init_swapchain();
    init_default_renderpass();
    init_framebuffers();
    init_pipelines();
}

void VulkanEngine::cleanup()
{	
	if (_isInitialized) {
        vkDeviceWaitIdle(_device->_logicalDevice);
        _renderFence->wait(1000000000);
        delete _renderFence;
        // Should I handle semaphores here? static queue (shared) maybe?
        delete _frameBuffers;
        delete _renderPass;
        delete _commandPool;
        delete _swapchain;
        _mainDeletionQueue.flush();

        // todo find a way to move this into vk_device without breaking swapchain
        vkDestroyDevice(_device->_logicalDevice, nullptr);
        vkDestroySurfaceKHR(_device->_instance, _device->_surface, nullptr);
        vkb::destroy_debug_utils_messenger(_device->_instance, _device->_debug_messenger);
        vkDestroyInstance(_device->_instance, nullptr);

        // glfwDestroyWindow(_window);
        delete _device;
        delete _window;
        glfwTerminate();
	}
}

void VulkanEngine::draw()
{
    // Wait GPU to render latest frame
    //wait until the GPU has finished rendering the last frame. Timeout of 1 second
//    VK_CHECK(vkWaitForFences(_device->_logicalDevice, 1, &_renderFence, true, 1000000000));
//    VK_CHECK(vkResetFences(_device->_logicalDevice, 1, &_renderFence));
    VK_CHECK(_renderFence->wait(1000000000));
    VK_CHECK(_renderFence->reset());

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_device->_logicalDevice, _swapchain->_swapchain, 1000000000, _presentSemaphore->_semaphore, nullptr, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) { // || result == VK_SUBOPTIMAL_KHR
        recreate_swap_chain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        throw std::runtime_error("failed to acquire swap chain image");
    }


    VK_CHECK(vkResetCommandBuffer(_commandBuffer->_mainCommandBuffer, 0));
    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr; // VkCommandBufferInheritanceInfo for secondary cmd buff. defines states inheriting from primary cmd. buff.
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(_commandBuffer->_mainCommandBuffer, &cmdBeginInfo));

    VkClearValue clearValue;
    float flash = abs(sin(_frameNumber / 120.f));
    clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };
    VkClearValue depthValue;
    depthValue.depthStencil.depth = 1.0f;
    std::array<VkClearValue, 2> clearValues = {clearValue, depthValue};

    VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(_renderPass->_renderPass, _window->_windowExtent, _frameBuffers->_frameBuffers[imageIndex]);
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = &clearValues[0];
    vkCmdBeginRenderPass(_commandBuffer->_mainCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

    draw_objects(_commandBuffer->_mainCommandBuffer, _renderables.data(), _renderables.size());
    vkCmdEndRenderPass(_commandBuffer->_mainCommandBuffer);

    VK_CHECK(vkEndCommandBuffer(_commandBuffer->_mainCommandBuffer));
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &(_presentSemaphore->_semaphore);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &(_renderSemaphore->_semaphore);
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffer->_mainCommandBuffer;

    VK_CHECK(vkQueueSubmit(_device->get_graphics_queue(), 1, &submitInfo, _renderFence->_fence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &(_swapchain->_swapchain);
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &(_renderSemaphore->_semaphore);
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