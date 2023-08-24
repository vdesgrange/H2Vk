/*
*  H2Vk - Shadow mapping
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <cstdint>
#include <vector>
#include "glm/gtc/matrix_transform.hpp"

#include "components/lighting/vk_light.h"
#include "core/vk_texture.h"
#include "core/vk_renderpass.h"
#include "core/vk_shaders.h"
#include "core/utilities/vk_types.h"

class Device;
class MaterialManager;
class RenderPass;
class FrameBuffer;
class DescriptorAllocator;
class DescriptorLayoutCache;
class Model;
class Node;

struct GPUShadowData {
    alignas(8) glm::vec2 num_lights;
    alignas(16) glm::mat4 directionalMVP[MAX_LIGHT];
    glm::mat4 spotMVP[MAX_LIGHT];
};

class ShadowMapping final {
public:
    bool debug = false;
    static const VkFormat DEPTH_FORMAT = VK_FORMAT_D16_UNORM;
    static const uint32_t SHADOW_WIDTH = 2048;
    static const uint32_t SHADOW_HEIGHT = 2048;

    Texture _offscreen_shadow;
    RenderPass _offscreen_pass;

    std::vector<FrameBuffer> _offscreen_framebuffer;
    std::vector<VkImageView> _offscreen_imageview;
    std::shared_ptr<Material> _offscreen_effect;
    std::shared_ptr<Material> _debug_effect;

    ShadowMapping() = delete;
    ShadowMapping(Device& device);
    ~ShadowMapping();

    static void allocate_buffers(Device& device);
    void prepare_depth_map(Device& device, UploadContext& uploadContext, LightingManager& lighting);
    void prepare_offscreen_pass(Device& device);
    void setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);
    void setup_offscreen_pipeline(Device& device, MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass);
    void run_offscreen_pass(FrameData& frame, Renderables& entities, LightingManager& lighting);
    void run_debug(FrameData& frame);
    GPUShadowData gpu_format(const LightingManager* lightingManager);

private:
    class Device& _device;

    void draw(Model& model, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance, bool bind);
    void draw_node(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance);

};