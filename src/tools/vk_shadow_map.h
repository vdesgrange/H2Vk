#pragma once

#include <cstdint>
#include "glm/gtc/matrix_transform.hpp"

#include "core/vk_texture.h"
#include "core/vk_renderpass.h"
#include "core/vk_shaders.h"

class Device;
class MaterialManager;
class RenderPass;
class DescriptorAllocator;
class DescriptorLayoutCache;

struct GPUDepthData {
    alignas(16) glm::mat4 depthMVP;
};

class ShadowMapping {
public:
    static const uint32_t SHADOW_WIDTH = 1024;
    static const uint32_t SHADOW_HEIGHT = 1024;
    Texture _offscreen_shadow;
    RenderPass _offscreen_pass;
    VkFramebuffer _offscreen_framebuffer;
    std::shared_ptr<Material> _offscreen_effect;

//    std::shared_ptr<Material> _offscreen;
//    std::shared_ptr<Material> _shadow;

    struct ImageMap {
        Texture texture;
        VkFramebuffer framebuffer;
        RenderPass render_pass;
    };

    ShadowMapping() = delete;
    ShadowMapping(Device& device);
    ~ShadowMapping();

    void prepare_depth_map(Device& device, UploadContext& uploadContext);
    static void allocate_buffers(Device& device);
    void setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout); // ShadowMapping::ImageMap shadowMap,
    std::shared_ptr<Material> setup_offscreen_pipeline(Device& device, MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass);
    std::shared_ptr<Material> setup_scene_pipeline(Device& device, MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass);
    static void render_pass(RenderPass& renderPass, VkFramebuffer& framebuffer, CommandBuffer& cmd, VkDescriptorSet& descriptor, std::shared_ptr<Material> offscreenPass);

private:
    class Device& _device;
};