#pragma once

class VulkanEngine;

#include "VkBootstrap.h"
#include "vk_types.h"

struct Statistics final
{
    VkExtent2D FramebufferSize;
    float FrameRate;
};

struct Settings final {
    bool p_open;
    bool p_overlay;
    int scene_index;

    // Camera
    int coordinates[3];
    float fov; // 0 - 360
    float aspect; // 0 - 1
    float z_near; // 0 - 1000, valeurs a la louche
    float z_far; // 0 - 1000, valeurs a la louche
};

class UInterface final {
public:
    VkDescriptorPool _imguiPool;

    UInterface(VulkanEngine& engine, Settings settings);
    ~UInterface();

    void init_imgui();
    void new_frame();
    void render(VkCommandBuffer cmd, Statistics stats);
    void demo();
    void interface();
    void interface_statistics(const Statistics& statistics);
    void clean_up();

    Settings& get_settings() { return _settings; };

private:
    class VulkanEngine& _engine;
    Settings _settings;
};