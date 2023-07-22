/*
*  H2Vk - Shader class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "vulkan/vulkan_core.h"
#include "core/manager/vk_system_manager.h"

#include <vector>
#include <iostream>
#include <memory>


class Device;

/**
 * @brief An enumeration of shader types
 * @note Vertex, tesselation control and evaluation, geometry, fragment and compute shaders
 */
enum struct ShaderType {
    VERTEX,
    TESSELATION_CONTROL,
    TESSELATION_EVALUATION,
    GEOMETRY,
    FRAGMENT,
    COMPUTE
};

/**
 * @brief Description of the push constant
 */
struct PushConstant {
    uint32_t size;
    ShaderType stage;
};

/**
 *
 * @brief Structure group a set of shaders composing a material/pipeline
 */
struct ShaderEffect {
    struct ShaderStage {
        VkShaderStageFlagBits flags;
        VkShaderModule shaderModule;
        VkSpecializationInfo specializationConstants;
    };

    std::vector<VkDescriptorSetLayout> setLayouts;
    std::vector<VkPushConstantRange> pushConstants;
    std::vector<ShaderStage> shaderStages;
};

/**
 * Built material composed by shader stages, pipeline and pipeline layout
 * @brief Shader built material
 * @note Material is an other name for ShaderPass
 */
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