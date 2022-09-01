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

class Helper final {
public:
    static std::vector<uint32_t> read_file(const char* filePath);
};