/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2024 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "vk_scene_listing.h"
#include <mutex>

class VulkanEngine;
class Camera;
class Texture;

class Scene final {
public:
    /** @brief a mutex */
    std::mutex _mutex;
    /** @brief Index of the scene */
    int _sceneIndex;
    /** @brief A vector of objects (model + material + transformation) */
    Renderables _renderables;
    /** @brief Resource synchronizer */
    bool _ready = false;

    explicit Scene(VulkanEngine& engine) : _engine(engine) {};

    void load_scene(int sceneIndex, Camera& camera);
    void render_objects(VkCommandBuffer commandBuffer, FrameData& frame);
    static void allocate_buffers(Device& device);
    void setup_transformation_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);
    void setup_texture_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);

private:
    VulkanEngine& _engine;
};