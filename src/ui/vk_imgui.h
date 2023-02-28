#pragma once

class VulkanEngine;

#include <unordered_map>
#include "VkBootstrap.h"
#include "core/utilities/vk_types.h"

typedef enum UIConstants {
    SCENE_EDITOR = 0,
    TEXTURE_VIEWER = 1,
    STATS_VIEWER = 2,
    SKYBOX_EDITOR = 3,
    VIEW_EDITOR = 4,
    LIGHT_EDITOR = 5,
    LOG_CONSOLE = 6,
} UIConstants;

struct Statistics final {
    VkExtent2D FramebufferSize;
    float FrameRate;
    float coordinates[3] {0.0f, 0.0f, 0.0f};
    float rotation[3] {0.0f, 0.0f, 0.0f};
};

struct Settings final {
    int scene_index;

    // Camera
    // int type {2};
    // float zNearFar[2] {0.010f, 200.0f};
    // float target[3] {0.0f, 0.0f, 0.0f};
    // float speed {10.0f};
    // float angle{70.f};

    // Light
    float coordinates[3] {1.0f, 0.0f, 0.0f};
    float colors[3] {1.0f, 1.0f, 1.0f};
    float ambient {0.0005};
    float specular {0.75};

    // Skybox
    bool display = false;
};

class UInterface final {
public:
    std::unordered_map<UIConstants, bool> p_open;
    VkDescriptorPool _pool;

    UInterface(VulkanEngine& engine, Settings settings);
    ~UInterface();

    void init_imgui();
    bool render(VkCommandBuffer cmd, Statistics stats);
    static bool want_capture_mouse();

    Settings& get_settings() { return _settings; };

private:
    class VulkanEngine& _engine;
    Settings _settings;

    void clean_up();
    void new_frame();

    bool interface(Statistics statistics);
    bool scene_editor();
    bool view_editor();
    bool texture_viewer();
    bool stats_viewer(const Statistics& statistics);
    bool skybox_editor();
    bool light_editor();
    bool log_console();
};