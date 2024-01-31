/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

class VulkanEngine;

#include <unordered_map>
#include <memory>
#include "VkBootstrap.h"
#include "core/vk_descriptor_cache.h"
#include "core/vk_descriptor_allocator.h"
#include "core/utilities/vk_performance.h"

typedef enum UIConstants {
    SCENE_EDITOR = 0,
    STATS_VIEWER = 1,
    SKYBOX_EDITOR = 2,
    VIEW_EDITOR = 3,
    LIGHT_EDITOR = 4,
    LOG_CONSOLE = 5,
    SHADOW_EDITOR = 6,
} UIConstants;

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
    bool render(VkCommandBuffer cmd, Performance::Statistics stats);
    static bool want_capture_mouse();

    Settings& get_settings() { return _settings; };

private:
    class VulkanEngine& _engine;
    Settings _settings;
    std::unique_ptr<DescriptorLayoutCache> _layoutCache;
    std::unique_ptr<DescriptorAllocator> _allocator;

    void clean_up();
    void new_frame();

    bool interface(Performance::Statistics statistics);
    bool scene_editor();
    bool view_editor();
    bool stats_viewer(const Performance::Statistics& statistics);
    bool skybox_editor();
    bool light_editor();
    bool shadow_editor();
};