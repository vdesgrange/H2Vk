#pragma once

#include "vulkan/vulkan_core.h"
#include "core/manager/vk_system_manager.h"

#include <vector>
#include <iostream>
#include <memory>


class Device;

enum struct ShaderType {
    VERTEX,
    TESSELATION_CONTROL,
    TESSELATION_EVALUATION,
    GEOMETRY,
    FRAGMENT,
    COMPUTE
};

struct PushConstant {
    uint32_t size;
    ShaderType stage;
};

struct ShaderEffect {
    struct ShaderStage {
        VkShaderStageFlagBits flags;
        VkShaderModule shaderModule;
    };

    std::vector<VkDescriptorSetLayout> setLayouts;
    std::vector<VkPushConstantRange> pushConstants;
    std::vector<ShaderStage> shaderStages;
};

struct ShaderPass : public Entity {
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::shared_ptr<ShaderEffect> effect;
};

typedef ShaderPass Material;

class Shader final {
public:
    static bool load_shader_module(const Device& device, const char* filePath, VkShaderModule* out);
    static bool create_shader_module(const Device& device, const std::vector<uint32_t>& code, VkShaderModule* out);
    static VkShaderStageFlagBits get_shader_stage(ShaderType type);
};