﻿#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <fstream>
#include <deque>
#include <functional>
#include <math.h>

#include "glm/gtc/matrix_transform.hpp"
#include "vk_types.h"
#include "vk_window.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_command_pool.h"
#include "vk_command_buffer.h"
#include "vk_renderpass.h"
#include "vk_framebuffers.h"
#include "vk_fence.h"
#include "vk_semaphore.h"
#include "vk_mesh.h"
#include "vk_material.h"
#include "vk_camera.h"
#include "vk_pipeline.h"


const bool enableValidationLayers = true;
const uint32_t MAX_FRAMES_IN_FLIGHT = 1;

struct RenderObject {
    Mesh* mesh;
    Material* material;
    glm::mat4 transformMatrix;
};

class VulkanEngine {
public:
	bool _isInitialized{ false };
	int _frameNumber {0};

    Window* _window;
    bool framebufferResized = false;
    Device* _device;
    SwapChain* _swapchain;

    CommandPool* _commandPool;
    CommandBuffer* _commandBuffer;
    RenderPass* _renderPass;
    FrameBuffers* _frameBuffers;

    Fence* _renderFence;
    Semaphore*  _presentSemaphore;
    Semaphore* _renderSemaphore;

    PipelineBuilder* _pipelineBuilder;

    std::vector<RenderObject> _renderables;
    std::unordered_map<std::string, Material> _materials;
    std::unordered_map<std::string, Mesh> _meshes;

    Mesh _mesh;
    Mesh _objMesh;

    DeletionQueue _mainDeletionQueue;

    Camera camera;

    int _selectedShader{ 0 };

    //initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

    void recreate_swap_chain();

	//draw loop
	void draw();

	//run main loop
	void run();

    Mesh* get_mesh(const std::string& name);

    void draw_objects(VkCommandBuffer commandBuffer, RenderObject* first, int count);

private:
    // Initialize vulkan
    void init_vulkan();

    void init_window();

    void init_scene();

    void init_camera();

    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    void init_swapchain();

    void init_commands();

    void init_default_renderpass();

    void init_framebuffers();

    void init_sync_structures();

    void init_pipelines();

    std::vector<uint32_t> read_file(const char* filePath);

    bool create_shader_module(const std::vector<uint32_t>& code, VkShaderModule* out);

    // bool load_shader_module(const char* filePath, VkShaderModule* out);

    void load_meshes();

    bool load_from_obj(const char* filename);

    void upload_mesh(Mesh& mesh);

};
