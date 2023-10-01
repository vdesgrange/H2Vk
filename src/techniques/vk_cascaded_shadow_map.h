/*
*  H2Vk - Cascaded shadow map
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <cstdint>
#include <vector>
#include <array>

#include "glm/mat4x4.hpp"
#include "core/utilities/vk_resources.h"
#include "core/utilities/vk_types.h"
#include "core/vk_texture.h"
#include "core/vk_device.h"
#include "core/vk_renderpass.h"
#include "core/vk_shaders.h"

class FrameData;
class Device;
class RenderPass;
class FrameBuffer;
class Camera;
class LightingManager;
class DescriptorLayoutCache;
class DescriptorAllocator;
class MaterialManager;

class CascadedShadow final {
public:
    bool _debug = false;
    const uint32_t SHADOW_WIDTH = 1024;
    const uint32_t SHADOW_HEIGHT = 1024;
    static const uint8_t COUNT = 3;
    const VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
    float _lb = 0.95f;
    int _cascadeIdx = 0;
    bool _colorCascades = false;

    struct Cascade {
        VkImageView _view;
        std::unique_ptr<FrameBuffer> _framebuffer;

        float splitDepth;
        glm::mat4 viewProjMatrix;
    };

    struct GPUCascadedShadowData {
        glm::mat4 VPMat[COUNT];
        glm::vec4 split;
        bool colorCascades;
    };

    std::array<Cascade, COUNT> _cascades;


    RenderPass _depthPass;
    Texture _depth;
    std::shared_ptr<Material> _depthEffect;
    std::shared_ptr<Material> _debugEffect;


    CascadedShadow(class Device& device);
    ~CascadedShadow();

    void prepare_resources(Device& device);
    static void allocate_buffers(Device& device);
    void setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);
    void setup_pipelines(Device& device, MaterialManager &materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass);
    void compute_resources(FrameData& frame, Renderables& renderables);
    void compute_cascades(Camera& camera, LightingManager& lightManager);
    void debug_depth(FrameData& frame);
    GPUCascadedShadowData gpu_format();

private:
    const class Device& _device;
};