#pragma once

class VulkanEngine;

#include "VkBootstrap.h"
#include "vk_types.h"

class UInterface final {
public:
    VkDescriptorPool _imguiPool;

    UInterface(VulkanEngine& engine) : _engine(engine) {};

    void init_imgui();
    void new_frame();
    void interface();
    void clean_up();

private:
    class VulkanEngine& _engine;
};