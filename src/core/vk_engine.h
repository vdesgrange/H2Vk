#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <fstream>
#include <deque>
#include <functional>
#include <cmath>

#include "glm/gtc/matrix_transform.hpp"
#include "vk_types.h"
#include "vk_helpers.h"
#include "vk_initializers.h"
#include "vk_fence.h"
#include "vk_mesh_manager.h"
#include "vk_scene_listing.h"
#include "vk_command_buffer.h"

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
class DescriptorPools;
class DescriptorLayoutCache;
class DescriptorAllocator;
class UInterface;
class ImDrawData;
class Statistics;
class Scene;
class SceneListing;
class TextureManager;
class SamplerManager;

struct Texture;

const bool enableValidationLayers = true;
constexpr unsigned int FRAME_OVERLAP = 2;

struct FrameData {
    Semaphore* _presentSemaphore;
    Semaphore* _renderSemaphore;
    Fence* _renderFence;

    CommandPool* _commandPool;
    CommandBuffer* _commandBuffer;

    AllocatedBuffer cameraBuffer;
    VkDescriptorSet environmentDescriptor;

    AllocatedBuffer objectBuffer;
    VkDescriptorSet objectDescriptor;
};

class VulkanEngine {
public:
	bool _isInitialized{ false };
	uint32_t _frameNumber {0};
    double _time = 0;
    bool framebufferResized = false;

    Window* _window;
    Device* _device;
    SwapChain* _swapchain;
    RenderPass* _renderPass;
    FrameBuffers* _frameBuffers;
    FrameData _frames[FRAME_OVERLAP];
    PipelineBuilder* _pipelineBuilder;
    std::vector<RenderObject> _renderables;
    MeshManager* _meshManager;
    TextureManager* _textureManager;
    SamplerManager* _samplerManager;
    SceneListing* _sceneListing;
    Scene* _scene;
    UInterface* _ui;

    DescriptorLayoutCache* _layoutCache;
    DescriptorAllocator* _allocator;

    struct {
        VkDescriptorSetLayout environment;
        VkDescriptorSetLayout matrices;
        VkDescriptorSetLayout textures;
    } _descriptorSetLayouts;

    DeletionQueue _mainDeletionQueue;

    Camera* _camera;
    GPUSceneData _sceneParameters;
    AllocatedBuffer _sceneParameterBuffer;
    UploadContext _uploadContext;

	void init();
	void cleanup();
	void draw();
	void run();

private:
    bool _reset {false};

    void init_vulkan();
    void init_window();
    void init_interface();
    void init_scene();
    void init_camera();
    void init_swapchain();
    void init_commands();
    void init_default_renderpass();
    void init_framebuffers();
    void init_sync_structures();
    void init_descriptors(); // can be call before choice of model
    void setup_descriptors(); // when switching model
    void init_pipelines();
    void load_meshes();
    void load_images();
    void recreate_swap_chain();
    void draw_objects(VkCommandBuffer commandBuffer, RenderObject* first, int count);
    void render(int imageIndex); // ImDrawData* draw_data,
    Statistics monitoring();
    FrameData& get_current_frame();
};
