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
#include "vk_fence.h"
#include "vk_mesh_manager.h"
#include "vk_scene_listing.h"

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

const bool enableValidationLayers = true;
constexpr unsigned int FRAME_OVERLAP = 2;

struct Texture {
    AllocatedImage image;
    VkImageView imageView;
};

struct PoolSize {
    std::vector<VkDescriptorPoolSize> sizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
    };
};

struct FrameData {
    Semaphore* _presentSemaphore;
    Semaphore* _renderSemaphore;
    Fence* _renderFence;

    CommandPool* _commandPool;
    CommandBuffer* _commandBuffer;

    AllocatedBuffer cameraBuffer;
    VkDescriptorSet globalDescriptor;

    AllocatedBuffer objectBuffer;
    VkDescriptorSet objectDescriptor;
};

class VulkanEngine {
public:
	bool _isInitialized{ false };
	uint32_t _frameNumber {0};
    double _time = 0;

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
    std::unordered_map<std::string, Texture> _loadedTextures;
    SceneListing* _sceneListing;
    Scene* _scene;
    UInterface* _ui;

    DescriptorLayoutCache* _layoutCache;
    DescriptorAllocator* _allocator;

    VkDescriptorSetLayout _globalSetLayout;
    VkDescriptorSetLayout _objectSetLayout;
    VkDescriptorSetLayout _singleTextureSetLayout;

    DeletionQueue _mainDeletionQueue;

    Camera* _camera;
    GPUSceneData _sceneParameters;
    AllocatedBuffer _sceneParameterBuffer;
    UploadContext _uploadContext;

    PoolSize poolSize;

    int _selectedShader{ 0 };

	void init();

	void cleanup();

    void recreate_swap_chain();

	void draw();

	void run();

    void draw_objects(VkCommandBuffer commandBuffer, RenderObject* first, int count);

    void load_images();

private:
    void init_vulkan();

    void init_window();

    void init_interface();

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

    void render(int imageIndex); // ImDrawData* draw_data,

    Statistics monitoring();

};
