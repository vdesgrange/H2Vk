#pragma once

class VulkanEngine;

#include <unordered_map>
#include "VkBootstrap.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_descriptor_allocator.h"

typedef enum UIConstants {
    SCENE_EDITOR = 0,
    STATS_VIEWER = 1,
    SKYBOX_EDITOR = 2,
    VIEW_EDITOR = 3,
    LIGHT_EDITOR = 4,
    LOG_CONSOLE = 5,
} UIConstants;

struct Statistics final {
    VkExtent2D FramebufferSize;
    float FrameRate;
    float coordinates[3] {0.0f, 0.0f, 0.0f};
    float rotation[3] {0.0f, 0.0f, 0.0f};
};

struct Settings final {
    int scene_index;

    // Scene
    int texture_index {-1};
    VkDescriptorSet _textureDescriptorSet {};

    // Skybox
    bool display = false;
};

class UInterface final {
public:
    std::unordered_map<UIConstants, bool> p_open;
    VkDescriptorPool _pool {};

    UInterface(VulkanEngine& engine, Settings settings);
    ~UInterface();

    void init_imgui();
    bool render(VkCommandBuffer cmd, Statistics stats);
    static bool want_capture_mouse();

    Settings& get_settings() { return _settings; };

private:
    class VulkanEngine& _engine;
    Settings _settings;
    DescriptorLayoutCache* _layoutCache;
    DescriptorAllocator* _allocator;

    void clean_up();
    void new_frame();

    bool interface(Statistics statistics);
    bool scene_editor();
    bool view_editor();
    bool stats_viewer(const Statistics& statistics);
    bool skybox_editor();
    bool light_editor();
};