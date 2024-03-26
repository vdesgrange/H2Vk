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
#include <mutex>

#include "glm/mat4x4.hpp"
#include "core/utilities/vk_resources.h"
#include "core/utilities/vk_types.h"
#include "core/vk_texture.h"
#include "core/vk_device.h"
#include "core/vk_renderpass.h"
#include "core/vk_shaders.h"
#include "core/vk_framebuffers.h"

class FrameData;
class Device;
class RenderPass;
class Camera;
class LightingManager;
class DescriptorLayoutCache;
class DescriptorAllocator;
class MaterialManager;

class CascadedShadow final {
public:
    /** @brief Display cascade layer with colors */
    bool _debug = false;
    /** @brief Shadow map width */
    const uint32_t SHADOW_WIDTH = 1024;
    /** @brief Shadow map height */
    const uint32_t SHADOW_HEIGHT = 1024;
    /** @brief Number of layers */
    static const uint8_t COUNT = 4;
    /** @brief Depth format */
    const VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
    /** @brief Split constant */
    float _lb = 0.95f;
    /** @brief Cascade index */
    int _cascadeIdx = 0;
    /** @brief Color cascades */
    bool _colorCascades = false;

    struct Cascade {
        VkImageView _view;
        std::unique_ptr<FrameBuffer> _framebuffer;
        VkDescriptorSet _descriptor;

        float splitDepth = 1.0f;
        glm::mat4 viewProjMatrix = glm::mat4(0.0f);

        void destroy(VkDevice device) {
            vkDestroyImageView(device, _view, nullptr);
            _framebuffer.reset();
        }
    };

    struct GPUCascadedShadowData {
        glm::mat4 VPMat[COUNT]; // alignas(glm::mat4)
        glm::vec4 split; // alignas(16)
        bool colorCascades; // alignas(bool)
    };

    std::array<Cascade, COUNT> _cascades;


    RenderPass _depthPass;
    Texture _depth;
    std::shared_ptr<Material> _depthEffect;
    std::shared_ptr<Material> _debugEffect;


    CascadedShadow(class Device& device, UploadContext& uploadContext);
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
    class UploadContext& _uploadContext;
    /** @brief Resource synchronizer */
    bool _ready = false;
};