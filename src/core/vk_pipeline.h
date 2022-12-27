#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "vk_types.h"
#include "vk_material.h"
#include "vk_shaders.h"

class Window;
class Device;
class RenderPass;

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;

    std::vector<ShaderPass> _shaderPasses; // duplicate with _materials
    std::unordered_map<std::string, std::shared_ptr<Material>> _materials;

    PipelineBuilder(const Window& window, const Device& device, RenderPass& renderPass, std::vector<VkDescriptorSetLayout> setLayouts);
    ~PipelineBuilder();

    bool load_shader_module(const char* filePath, VkShaderModule* out);

    bool create_shader_module(const std::vector<uint32_t>& code, VkShaderModule* out);

    VkPipeline build_pipeline(const Device& device, RenderPass& renderPass);

    std::shared_ptr<Material> create_material(VkPipeline pipeline, VkPipelineLayout pipelineLayout, const std::string &name);

    std::shared_ptr<Material> get_material(const std::string &name);

private:
    const class Device& _device;
    class RenderPass& _renderPass;

    VkPipelineLayout* _pipelineLayout;

    ShaderPass build_pass(ShaderEffect* effect);

    ShaderEffect build_effect(std::vector<VkDescriptorSetLayout> setLayouts,  std::vector<VkPushConstantRange> pushConstants, std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules);

    VkPipelineLayout build_layout(std::vector<VkDescriptorSetLayout> setLayouts, std::vector<VkPushConstantRange> pushConstants);

    void set_shaders(ShaderEffect& effect);

    void scene_monkey_triangle(std::vector<VkDescriptorSetLayout> setLayouts);

    void scene_lost_empire(std::vector<VkDescriptorSetLayout> setLayouts);

    void scene_old_bridge(std::vector<VkDescriptorSetLayout> setLayouts);

    void scene_karibu_hippo(std::vector<VkDescriptorSetLayout> setLayouts);

    void scene_damaged_helmet(std::vector<VkDescriptorSetLayout> setLayouts);
};