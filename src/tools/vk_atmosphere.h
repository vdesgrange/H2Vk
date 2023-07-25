#pragma once

#include <cstdint>
#include <array>

#include "core/vk_texture.h"

class Device;
class DescriptorLayoutCache;
class DescriptorAllocator;
struct FrameData;
struct UploadContext;

class Atmosphere final {
public:
    bool debug = false;

    const uint32_t TRANSMITTANCE_WIDTH = 256;
    const uint32_t TRANSMITTANCE_HEIGHT = 64;

    const uint32_t MULTISCATTERING_WIDTH = 32;
    const uint32_t MULTISCATTERING_HEIGHT = 32;

    const uint32_t SKYVIEW_WIDTH = 200;
    const uint32_t SKYVIEW_HEIGHT = 100;

    Texture _transmittanceLUT;
    Texture _transmittanceLUT2;
    VkFramebuffer _transmittanceFramebuffer;

    Texture _multipleScatteringLUT;

    Texture _skyviewLUT;
    VkFramebuffer _skyviewFramebuffer;

    Atmosphere() =  delete;
    Atmosphere(Device& device, UploadContext& uploadContext);
    ~Atmosphere();

    void setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);
    Texture compute_transmittance(Device& device, UploadContext& uploadContext);
    Texture compute_transmittance_2(Device& device, UploadContext& uploadContext);
    Texture compute_multiple_scattering(Device& device, UploadContext& uploadContext);
    Texture compute_skyview(Device& device, UploadContext& uploadContext);
    void run_debug(FrameData& frame);

private:
    class Device& _device;
    class UploadContext& _uploadContext;

    void draw();
};