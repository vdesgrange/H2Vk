#pragma once

#include <cstdint>
#include <array>
#include <glm/vec4.hpp>

#include "core/vk_texture.h"
#include "core/vk_shaders.h"
#include "core/vk_renderpass.h"
#include "core/vk_framebuffers.h"

class Device;
class LightingManager;
class RenderPass;
class FrameBuffer;
class MaterialManager;
class DescriptorLayoutCache;
class DescriptorAllocator;
struct FrameData;
struct UploadContext;

class Atmosphere final {
public:
    bool _debug = true;

    const uint32_t TRANSMITTANCE_WIDTH = 256;
    const uint32_t TRANSMITTANCE_HEIGHT = 64;

    const uint32_t MULTISCATTERING_WIDTH = 32;
    const uint32_t MULTISCATTERING_HEIGHT = 32;

    const uint32_t SKYVIEW_WIDTH = 200;
    const uint32_t SKYVIEW_HEIGHT = 100;

    Texture _transmittanceLUT;
    std::shared_ptr<Material> _transmittancePass; // weak_ptr?
    VkDescriptorSet _transmittanceDescriptor;
    VkDescriptorSetLayout _transmittanceDescriptorLayout;

    Texture _multipleScatteringLUT;
    std::shared_ptr<Material> _multiScatteringPass; // weak_ptr?
    RenderPass _multipleScatteringRenderPass;
    std::unique_ptr<FrameBuffer> _multipleScatteringFramebuffer;
    VkDescriptorSet _multipleScatteringDescriptor;
    VkDescriptorSetLayout _multipleScatteringDescriptorLayout;

    Texture _skyviewLUT;
    std::shared_ptr<Material> _skyviewPass; // weak_ptr?
    RenderPass _skyviewRenderPass;
    std::unique_ptr<FrameBuffer> _skyviewFramebuffer;
    std::vector<VkDescriptorSet> _skyviewDescriptor;
    VkDescriptorSetLayout _skyviewDescriptorLayout;

    Texture _atmosphereLUT;
    std::shared_ptr<Material> _atmospherePass;
    VkDescriptorSetLayout _atmosphereDescriptorLayout;

    Atmosphere() =  delete;
    Atmosphere(Device& device, MaterialManager& materialManager, LightingManager& lightingManager, UploadContext& uploadContext);
    ~Atmosphere();

    void create_resources(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, RenderPass& renderPass);
    void precompute_resources();
    void compute_resources(uint32_t frameIndex);
    void destroy_resources();

    void draw(VkCommandBuffer& commandBuffer, VkDescriptorSet* descriptor);

private:
    class Device& _device;
    class MaterialManager& _materialManager;
    class LightingManager& _lightingManager;
    class UploadContext& _uploadContext;

    void create_transmittance_resource(Device& device, UploadContext& uploadContext, DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator);
    void create_multiple_scattering_resource(Device& device, UploadContext& uploadContext, DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator);
    void create_skyview_resource(Device& device, UploadContext& uploadContext, DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator);
    void create_atmosphere_resource(Device& device, UploadContext& uploadContext, DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, RenderPass& renderPass);

    void compute_transmittance(Device& device, UploadContext& uploadContext, CommandBuffer& commandBuffer);
    void compute_multiple_scattering(Device& device, UploadContext& uploadContext, CommandBuffer& commandBuffer);
    void compute_skyview(Device& device, UploadContext& uploadContext, uint32_t frameIndex, glm::vec4 sun_direction = glm::vec4(0.0));
};