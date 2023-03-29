#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <fstream>
#include <deque>
#include <functional>
#include <cmath>
#include <memory>

#include "core/utilities/vk_resources.h"
#include "core/utilities/vk_global.h"
#include "core/utilities/vk_initializers.h"

#include "core/vk_window.h"
#include "core/vk_device.h"
#include "core/vk_swapchain.h"
#include "core/vk_renderpass.h"
#include "core/vk_framebuffers.h"
#include "core/vk_pipeline.h"
#include "core/vk_descriptor_allocator.h"
#include "core/vk_descriptor_builder.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_fence.h"
#include "core/vk_semaphore.h"
#include "core/vk_texture.h"
#include "core/vk_command_pool.h"
#include "core/vk_command_buffer.h"
#include "core/vk_buffer.h"

#include "core/manager/vk_material_manager.h"
#include "core/manager/vk_mesh_manager.h"

#include "core/skybox/vk_skybox.h"
#include "core/camera/vk_camera.h"
#include "core/model/vk_model.h"
#include "core/lighting/vk_light.h"

#include "ui/vk_imgui.h"

#include "scenes/vk_scene_listing.h"
#include "scenes/vk_scene.h"

class Window;
class Device;
class SwapChain;
class RenderPass;
class FrameBuffers;
class GraphicPipeline;

class SceneListing;
class Scene;
class UInterface;
class Skybox;

class SystemManager;
class MaterialManager;
class MeshManager;
class LightingManager;
class Camera;

class CommandPool;
class CommandBuffer;

class Semaphore;
class Fence;
class DescriptorLayoutCache;
class DescriptorAllocator;
class Statistics;

const bool enableValidationLayers = true;

class VulkanEngine {
public:
	bool _isInitialized = false;
    uint32_t _frameNumber = 0;
    double _time = 0;

    std::unique_ptr<Window> _window;
    std::unique_ptr<Device> _device;
    std::unique_ptr<SwapChain> _swapchain;
    std::unique_ptr<RenderPass> _renderPass;
    std::unique_ptr<FrameBuffers> _frameBuffers;
    std::unique_ptr<GraphicPipeline> _pipelineBuilder;

    std::unique_ptr<SceneListing> _sceneListing;
    std::unique_ptr<Scene> _scene;
    std::unique_ptr<UInterface> _ui;
    std::unique_ptr<Skybox> _skybox;

    std::unique_ptr<SystemManager> _systemManager;
    std::shared_ptr<MaterialManager> _materialManager;
    std::shared_ptr<MeshManager> _meshManager;
    std::shared_ptr<LightingManager> _lightingManager;
    std::unique_ptr<Camera> _camera; // std::shared_ptr<class CameraManager> _cameraManager; todo

    // FrameData _frames[FRAME_OVERLAP];
    std::vector<RenderObject> _renderables;

    DescriptorLayoutCache* _layoutCache;
    DescriptorAllocator* _allocator;

    struct {
        VkDescriptorSetLayout skybox;
        VkDescriptorSetLayout environment;
        VkDescriptorSetLayout matrices;
        VkDescriptorSetLayout textures;
        VkDescriptorSetLayout gui;
    } _descriptorSetLayouts;

    DeletionQueue _mainDeletionQueue;

    UploadContext _uploadContext;

	void init();
	void cleanup();
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
    void init_descriptors();
    void setup_environment_descriptors();
    void init_materials();
    void init_managers();
    void recreate_swap_chain();
    void ui_overlay();
    void update_uniform_buffers();
    void update_buffer_objects(RenderObject *first, int count);
    void render_objects(VkCommandBuffer commandBuffer);
    void build_command_buffers(FrameData frame, int imageIndex);
    void render(int imageIndex);
    void draw();
    Statistics monitoring();
    FrameData& get_current_frame();
};
