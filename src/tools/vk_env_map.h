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
class SwapChain;
struct UploadContext;

class EnvMap final {
public:
    int texWidth, texHeight, texChannels;

    void loader(const char* file, Device& device, Texture& texture, UploadContext& uploadContext);
    void cube_map_converter(const char* file, Window& window, Device& device, SwapChain& swapchain, Texture& texture);
    void main();
};