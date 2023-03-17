#pragma once

#include <iostream>
#include <fstream>
#include <vector>

#define VK_CHECK(x) \
    do \
    { \
        VkResult err = x; \
        if (err) { \
            std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort(); \
        } \
    } while (0)

class Device;

namespace helper {
    std::vector<uint32_t> read_file(const char* filePath);
    size_t pad_uniform_buffer_size(const Device& device, size_t originalSize);
}