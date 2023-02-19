#pragma once

#include <memory>
#include <iostream>
#include <array>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "vk_mem_alloc.h"
#include "stb_image.h"
#include "stb_image_write.h"

class Window;
class Device;
class Texture;
class MeshManager;
class SwapChain;
struct UploadContext;

class EnvMap final {
public:
    const uint32_t ENV_WIDTH = 1600;
    const uint32_t ENV_HEIGHT = 1600;

    int texWidth, texHeight, texChannels;

    void main();
    void clean_up();
    void loader(const char* file, Device& device, Texture& texture, UploadContext& uploadContext);
    void init_pipeline();
    Texture cube_map_converter(Device& device, UploadContext& uploadContext, MeshManager& meshManager, Texture& inTexture); // , Texture& outTexture
    Texture irradiance_mapping(Window& window, Device& device, UploadContext& uploadContext, Texture& inTexture);

    void prefilter_mapping();
    void brdf_convolution();
};