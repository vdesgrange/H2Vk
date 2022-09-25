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
};

class UInterface final {
public:
    VkDescriptorPool _imguiPool;

    UInterface(VulkanEngine& engine, Settings settings);
    ~UInterface();

    void init_imgui();
    void new_frame();
    void render(VkCommandBuffer cmd);
    void demo();
    void interface();
    void statistics(const Statistics& statistics);
    void clean_up();

    Settings& get_settings() { return _settings; };

private:
    class VulkanEngine& _engine;
    Settings _settings;
};