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

    const uint32_t CONVOLVE_WIDTH = 60;
    const uint32_t CONVOLVE_HEIGHT = 60;

    const uint32_t PRE_FILTER_WIDTH = 60;
    const uint32_t PRE_FILTER_HEIGHT = 60;


    Texture cube_map_converter(Device& device, UploadContext& uploadContext, MeshManager& meshManager, Texture& inTexture);
    Texture irradiance_mapping(Device& device, UploadContext& uploadContext, Texture& inTexture);
    Texture irradiance_cube_mapping(Device& device, UploadContext& uploadContext, MeshManager& meshManager, Texture& inTexture);
    Texture prefilter_cube_mapping(Device& device, UploadContext& uploadContext, MeshManager& meshManager, Texture& inTexture);
    Texture brdf_convolution();
};