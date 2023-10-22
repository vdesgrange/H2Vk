/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

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
