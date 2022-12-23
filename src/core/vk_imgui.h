#pragma once

class VulkanEngine;

#include <unordered_map>
#include "VkBootstrap.h"
#include "vk_types.h"

struct Statistics final
{
    VkExtent2D FramebufferSize;
    float FrameRate;
};

struct Settings final {
    int scene_index;

    // Camera
    int coordinates[3];
    float speed {10.0f};
    float fov{70.f}; // 0 - 360
    float aspect {1700.f / 1200.f}; // 0 - 1
    float z_near {0.1f}; // 0 - 1000, valeurs a la louche
    float z_far {200.0f}; // 0 - 1000, valeurs a la louche
};

class UInterface final {
public:
    std::unordered_map<std::string, bool> p_open;
    VkDescriptorPool _imguiPool;

    UInterface(VulkanEngine& engine, Settings settings);
    ~UInterface();

    void init_imgui();
    void new_frame();
    void render(VkCommandBuffer cmd, Statistics stats);
    void interface(Statistics statistics);
    void scene_editor();
    void texture_viewer();
    void stats_viewer(const Statistics& statistics);
    void clean_up();
    bool want_capture_mouse() const;

    Settings& get_settings() { return _settings; };

private:
    class VulkanEngine& _engine;
    Settings _settings;
};