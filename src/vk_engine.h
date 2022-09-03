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
#include "vk_helpers.h"
#include "vk_initializers.h"

class Window;
class Device;
class SwapChain;
class CommandPool;
class CommandBuffer;
class RenderPass;
class FrameBuffers;
class Fence;
class Semaphore;
class PipelineBuilder;
class MeshManager;
class DeletionQueue;
class Camera;
class Material;
class Mesh;

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
    MeshManager* _meshManager;

    DeletionQueue _mainDeletionQueue;

    Camera* _camera;

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

    void draw_objects(VkCommandBuffer commandBuffer, RenderObject* first, int count);

private:
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

    void load_meshes();
};
