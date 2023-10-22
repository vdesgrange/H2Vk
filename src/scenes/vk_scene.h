/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "vk_scene_listing.h"

class VulkanEngine;
class Camera;
class Texture;

class Scene final {
public:
    int _sceneIndex;
    Renderables _renderables;

    explicit Scene(VulkanEngine& engine) : _engine(engine) {};

    void load_scene(int sceneIndex, Camera& camera);
    void render_objects(VkCommandBuffer commandBuffer, FrameData& frame);
    static void allocate_buffers(Device& device);
    void setup_transformation_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);
    void setup_texture_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);
private:
    VulkanEngine& _engine;
};