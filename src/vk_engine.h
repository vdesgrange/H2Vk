#pragma once

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
#include "vk_mesh.h"
#include "vk_camera.h"

//const uint32_t CWIDTH = 800;
//const uint32_t CHEIGHT = 600;

const bool enableValidationLayers = true;
const uint32_t MAX_FRAMES_IN_FLIGHT = 1;

//const std::vector<const char*> deviceExtensions = {
//        VK_KHR_SWAPCHAIN_EXTENSION_NAME
//};

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};

struct Material {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

struct RenderObject {
    Mesh* mesh;
    Material* material;
    glm::mat4 transformMatrix;
};

//struct DeletionQueue
//{
//    std::deque<std::function<void()>> deletors;
//
//    void push_function(std::function<void()>&& function) {
//        deletors.push_back(function);
//    }
//
//    void flush() {
//        // reverse iterate the deletion queue to execute all the functions
//        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
//            (*it)(); //call the function
//        }
//
//        deletors.clear();
//    }
//};

class VulkanEngine {
public:
	bool _isInitialized{ false };
	int _frameNumber {0};

    Window* _window;
    bool framebufferResized = false;

    Device* _device;
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;;

    VkRenderPass _renderPass;
    std::vector<VkFramebuffer> _frameBuffers;

    SwapChain* _swapchain;

    VkSemaphore _presentSemaphore, _renderSemaphore;
    VkFence _renderFence;

    VkPipelineLayout _pipelineLayout;
    VkPipelineLayout _meshPipelineLayout;

    VkPipeline _graphicsPipeline;
    VkPipeline _redTrianglePipeline;
    VkPipeline _meshPipeline;

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

    Material* create_material(VkPipeline pipeline, VkPipelineLayout pipelineLayout, const std::string& name);

    Material* get_material(const std::string& name);

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

    void init_command_pool();

    void init_command_buffer();

    void init_default_renderpass();

    void init_framebuffers();

    void init_sync_structures();

    void init_pipelines();

    std::vector<uint32_t> read_file(const char* filePath);

    bool create_shader_module(const std::vector<uint32_t>& code, VkShaderModule* out);

    bool load_shader_module(const char* filePath, VkShaderModule* out);

    void load_meshes();

    bool load_from_obj(const char* filename);

    void upload_mesh(Mesh& mesh);

};
