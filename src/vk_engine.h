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
constexpr unsigned int FRAME_OVERLAP = 2;

struct RenderObject {
    Mesh* mesh;
    Material* material;
    glm::mat4 transformMatrix;
};

struct GPUSceneData {
    glm::vec4 fogColor;
    glm::vec4 fogDistance;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;
};

struct FrameData {
    Semaphore* _presentSemaphore;
    Semaphore* _renderSemaphore;
    Fence* _renderFence;

    CommandPool* _commandPool;
    CommandBuffer* _commandBuffer;

    AllocatedBuffer cameraBuffer;
    VkDescriptorSet globalDescriptor;
};

class VulkanEngine {
public:
	bool _isInitialized{ false };
	uint32_t _frameNumber {0};

    Window* _window;
    bool framebufferResized = false;
    Device* _device;
    SwapChain* _swapchain;
    RenderPass* _renderPass;
    FrameBuffers* _frameBuffers;
    FrameData _frames[FRAME_OVERLAP];
    PipelineBuilder* _pipelineBuilder;
    std::vector<RenderObject> _renderables;
    MeshManager* _meshManager;

    VkDescriptorSetLayout _globalSetLayout;
    VkDescriptorPool _descriptorPool;

    DeletionQueue _mainDeletionQueue;

    Camera* _camera;
    GPUSceneData _sceneParameters;
    AllocatedBuffer _sceneParameterBuffer;

    int _selectedShader{ 0 };

	void init();

	void cleanup();

    void recreate_swap_chain();

	void draw();

	void run();

    void draw_objects(VkCommandBuffer commandBuffer, RenderObject* first, int count);

private:
    void init_vulkan();

    void init_window();

    void init_scene();

    void init_camera();

    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    FrameData& get_current_frame();

    void init_swapchain();

    void init_commands();

    void init_default_renderpass();

    void init_framebuffers();

    void init_sync_structures();

    void init_descriptors();

    void init_pipelines();

    void load_meshes();

    size_t pad_uniform_buffer_size(size_t originalSize);
};
