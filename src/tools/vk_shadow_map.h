#pragma once

#include <cstdint>
#include "glm/gtc/matrix_transform.hpp"

#include "core/vk_texture.h"
#include "core/vk_renderpass.h"
#include "core/vk_shaders.h"
#include "core/utilities/vk_types.h"

class Device;
class MaterialManager;
class RenderPass;
class DescriptorAllocator;
class DescriptorLayoutCache;
class Model;
class Node;

struct GPUDepthData {
    glm::mat4 depthMVP;
};

class ShadowMapping final {
public:
    bool debug = false;
    static const VkFormat DEPTH_FORMAT = VK_FORMAT_D16_UNORM;
    static const uint32_t SHADOW_WIDTH = 2048;
    static const uint32_t SHADOW_HEIGHT = 2048;

    Texture _offscreen_shadow;
    RenderPass _offscreen_pass;
    VkFramebuffer _offscreen_framebuffer;
    std::shared_ptr<Material> _offscreen_effect;
    std::shared_ptr<Material> _debug_effect;

    ShadowMapping() = delete;
    ShadowMapping(Device& device);
    ~ShadowMapping();

    static void allocate_buffers(Device& device);
    void prepare_depth_map(Device& device, UploadContext& uploadContext, RenderPass& renderPass);
    void prepare_offscreen_pass(Device& device);
    void setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);
    void setup_offscreen_pipeline(Device& device, MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass);
    void run_offscreen_pass(FrameData& frame, Renderables& entities);
    void run_debug(FrameData& frame);
private:
    class Device& _device;

    void draw(Model& model, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance, bool bind);
    void draw_node(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance);

};