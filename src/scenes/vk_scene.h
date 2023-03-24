#pragma once

#include "vk_scene_listing.h"

class VulkanEngine;
class Camera;

class Scene final {
public:
    int _sceneIndex;
    Renderables _renderables;

    explicit Scene(VulkanEngine& engine) : _engine(engine) {};

    void load_scene(int sceneIndex, Camera& camera);
    void setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);

private:
    VulkanEngine& _engine;
};