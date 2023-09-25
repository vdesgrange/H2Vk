/*
*  H2Vk - Cascaded shadow mapping
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include "glm/gtc/matrix_transform.hpp"

#include "components/lighting/vk_light.h"
#include "core/vk_framebuffers.h"
#include "core/vk_texture.h"
#include "core/vk_renderpass.h"
#include "core/vk_shaders.h"
#include "core/utilities/vk_types.h"

class FrameBuffer;
class Camera;
class MaterialManager;
class Node;

struct GPUCascadedShadowData {
    alignas(glm::mat4) glm::mat4 cascadeVP[3];
    alignas(glm::vec4) glm::vec4 splitDepth;
    alignas(bool) bool colorCascades;
};


class CascadedShadowMapping final {
public:
    static const VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
    const uint32_t SHADOW_WIDTH = 2048;
    const uint32_t SHADOW_HEIGHT = 2048;
    static const uint32_t CASCADE_COUNT = 3;

    struct Cascade {
        VkImageView _imageView;
        std::unique_ptr<FrameBuffer> _frameBuffer;
        VkDescriptorSet _descriptorSet;

        float splitDepth;
        glm::mat4 viewProj;
    };

    struct DirectionalShadow {
        std::array<Cascade, CASCADE_COUNT> _cascades;
    };

    bool _debug_depth_map = false;
    bool _color_cascades = false;
    float _lb = 0.95f; // split lambda
    int _cascade_idx = 0;

    Texture _offscreen_shadow;
    RenderPass _offscreen_pass;
    std::shared_ptr<Material> _offscreen_effect = nullptr;
    std::shared_ptr<Material> _debug_effect = nullptr;

    // std::vector<DirectionalShadow> _directional_shadows;
    DirectionalShadow _directional_shadows {};
//    std::vector<VkImageView> _cascades_imageview;
//    std::vector<FrameBuffer> _cascades_framebuffer;
//    std::array<VkDescriptorSet, CASCADE_COUNT> _cascades_descriptor;

    CascadedShadowMapping() = delete;
    CascadedShadowMapping(Device& device);
    ~CascadedShadowMapping();

    static void allocate_buffers(Device& device);
    void prepare_depth_map(Device& device, UploadContext& uploadContext, LightingManager& lighting);
    void setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);
    void setup_pipelines(Device& device, MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass);
    void run_offscreen_pass(FrameData& frame, Renderables& entities, LightingManager& lighting);
    void run_debug(FrameData& frame);
    
    void compute_cascades(Camera& camera, LightingManager& lighting);

    GPUCascadedShadowData gpu_format();

private:
    class Device& _device;

    void prepare_offscreen_pass(Device& device);
    void draw(Model& model, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance, bool bind);
    void draw_node(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance);

};

