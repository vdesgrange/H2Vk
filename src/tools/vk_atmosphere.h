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

    Texture _transmittanceLUT;
    VkFramebuffer _transmittanceFramebuffer;

    Atmosphere() =  delete;
    Atmosphere(Device& device, UploadContext& uploadContext);
    ~Atmosphere();

    void setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);
    Texture compute_transmittance(Device& device, UploadContext& uploadContext);
    void run_debug(FrameData& frame);

private:
    class Device& _device;
    class UploadContext& _uploadContext;

    void draw();
};