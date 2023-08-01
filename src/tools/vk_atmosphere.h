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

    const uint32_t ATMOSPHERE_WIDTH = 512;
    const uint32_t ATMOSPHERE_HEIGHT = 256;

    Texture _transmittanceLUT;

    Texture _multipleScatteringLUT;
    VkFramebuffer _multipleScatteringFramebuffer;

    Texture _skyviewLUT;
    VkFramebuffer _skyviewFramebuffer;

    Texture _atmosphereLUT;
    VkFramebuffer _atmosphereFramebuffer;

    Atmosphere() =  delete;
    Atmosphere(Device& device, UploadContext& uploadContext);
    ~Atmosphere();

    Texture compute_transmittance(Device& device, UploadContext& uploadContext);
    Texture compute_multiple_scattering(Device& device, UploadContext& uploadContext);
    Texture compute_multiple_scattering_2(Device& device, UploadContext& uploadContext);
    Texture compute_skyview(Device& device, UploadContext& uploadContext);
    Texture render_atmosphere(Device& device, UploadContext& uploadContext);

    void run_debug(FrameData& frame);

private:
    class Device& _device;
    class UploadContext& _uploadContext;
};