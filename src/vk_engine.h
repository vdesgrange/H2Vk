﻿#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <fstream>
#include <deque>
#include <functional>
#include <cmath>
#include <memory>

#include "vk_mem_alloc.h"
#include "VkBootstrap.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"

#include "ui/vk_imgui.h"
#include "core/model/vk_model.h"
#include "core/skybox/vk_skybox.h"
#include "core/lighting/vk_light.h"
#include "core/vk_buffer.h"
#include "core/camera/vk_camera.h"
#include "core/vk_command_buffer.h"
#include "core/vk_command_pool.h"
#include "core/vk_descriptor_allocator.h"
#include "core/vk_descriptor_builder.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_device.h"
#include "core/vk_fence.h"
#include "core/vk_framebuffers.h"
#include "core/utilities/vk_helpers.h"
#include "core/utilities/vk_initializers.h"
#include "core/manager/vk_mesh_manager.h"
#include "core/manager/vk_material_manager.h"
#include "core/vk_pipeline.h"
#include "core/vk_renderpass.h"
#include "scenes/vk_scene.h"
#include "scenes/vk_scene_listing.h"
#include "core/vk_semaphore.h"
#include "core/vk_swapchain.h"
#include "core/vk_texture.h"
#include "core/utilities/vk_types.h"
#include "core/vk_window.h"

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
class Mesh;
class DescriptorLayoutCache;
class DescriptorAllocator;
class UInterface;
class ImDrawData;
class Statistics;
class Scene;
class SceneListing;
class Skybox;

struct Texture;

const bool enableValidationLayers = true;
constexpr unsigned int FRAME_OVERLAP = 2;

struct FrameData {
    Semaphore* _presentSemaphore;
    Semaphore* _renderSemaphore;
    Fence* _renderFence;

    CommandPool* _commandPool;
    CommandBuffer* _commandBuffer;

    VkDescriptorSet skyboxDescriptor;

    AllocatedBuffer cameraBuffer;
    AllocatedBuffer lightingBuffer;
    VkDescriptorSet environmentDescriptor;

    AllocatedBuffer objectBuffer;
    VkDescriptorSet objectDescriptor;
};

class VulkanEngine {
public:
	bool _isInitialized = false;
    bool _skyboxDisplay = true;
    uint32_t _frameNumber = 0;
    double _time = 0;

    std::unique_ptr<class Window> _window;
    std::unique_ptr<class Device> _device;
    std::unique_ptr<class SwapChain> _swapchain;
    std::unique_ptr<class RenderPass> _renderPass;
    std::unique_ptr<class FrameBuffers> _frameBuffers;
    std::unique_ptr<class GraphicPipeline> _pipelineBuilder;

    std::unique_ptr<class SceneListing> _sceneListing;
    std::unique_ptr<class Scene> _scene;
    std::unique_ptr<class UInterface> _ui;
    std::unique_ptr<class Skybox> _skybox;

    std::unique_ptr<class SystemManager> _systemManager;
    std::shared_ptr<class MaterialManager> _materialManager;
    std::shared_ptr<class MeshManager> _meshManager;
    std::shared_ptr<class LightingManager> _lightingManager;
    std::unique_ptr<class Camera> _camera; // std::shared_ptr<class CameraManager> _cameraManager; todo

    FrameData _frames[FRAME_OVERLAP];
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
    void init_descriptors(); // can be call before choice of model
    void setup_descriptors(); // when switching model
    void init_materials();
    void init_managers();
    void recreate_swap_chain();
    void skybox(VkCommandBuffer commandBuffer);
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
