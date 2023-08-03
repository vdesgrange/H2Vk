#pragma once

#include <cstdint>
#include <array>

#include "core/vk_texture.h"
#include "core/vk_shaders.h"

class Device;
class RenderPass;
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

    const uint32_t ATMOSPHERE_WIDTH = 1280;
    const uint32_t ATMOSPHERE_HEIGHT = 720;

    Texture _transmittanceLUT;

    Texture _multipleScatteringLUT;
    VkFramebuffer _multipleScatteringFramebuffer;

    Texture _skyviewLUT;
    VkFramebuffer _skyviewFramebuffer;

    Texture _atmosphereLUT;
    VkFramebuffer _atmosphereFramebuffer;
    std::shared_ptr<Material> _atmospherePass;

    Atmosphere() =  delete;
    Atmosphere(Device& device, UploadContext& uploadContext);
    ~Atmosphere();

    void precompute_lut();
    void compute_resources();
    Texture compute_transmittance(Device& device, UploadContext& uploadContext);
    Texture compute_multiple_scattering(Device& device, UploadContext& uploadContext);
    Texture compute_multiple_scattering_2(Device& device, UploadContext& uploadContext);
    Texture compute_skyview(Device& device, UploadContext& uploadContext);
    Texture render_atmosphere(Device& device, UploadContext& uploadContext);

    void setup_atmosphere_descriptor(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);
    void debug_descriptor(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout, MaterialManager& materialManager, RenderPass& renderPass);
    void setup_material(MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts, RenderPass& renderPass);
    void draw(VkCommandBuffer& commandBuffer, VkDescriptorSet* descriptor);

private:
    class Device& _device;
    class UploadContext& _uploadContext;
};