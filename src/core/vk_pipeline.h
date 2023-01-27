#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "core/utilities/vk_types.h"
#include "vk_shaders.h"

class Window;
class Device;
class RenderPass;

class PipelineBuilder {
public:
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    std::unordered_map<std::string, std::shared_ptr<Material>> _materials;

    PipelineBuilder(const Window& window, const Device& device, RenderPass& renderPass);
    ~PipelineBuilder();

    std::shared_ptr<ShaderPass> get_material(const std::string &name);
    void skybox(std::vector<VkDescriptorSetLayout> setLayouts);
    void scene_light(std::vector<VkDescriptorSetLayout> setLayouts);
    void scene_monkey_triangle(std::vector<VkDescriptorSetLayout> setLayouts);
    void scene_karibu_hippo(std::vector<VkDescriptorSetLayout> setLayouts);
    void scene_damaged_helmet(std::vector<VkDescriptorSetLayout> setLayouts);

    std::shared_ptr<ShaderEffect> build_effect(std::vector<VkDescriptorSetLayout> setLayouts,  std::vector<VkPushConstantRange> pushConstants, std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> shaderModules);
    std::shared_ptr<ShaderPass> build_pass(std::shared_ptr<ShaderEffect> effect);
    void create_material(const std::string &name, std::shared_ptr<Material> pass);

private:
    const class Device& _device;
    const class RenderPass& _renderPass;

    VkPipelineLayout build_layout(std::vector<VkDescriptorSetLayout> &setLayouts, std::vector<VkPushConstantRange> &pushConstants) const;
    VkPipeline build_pipeline(const Device& device, const RenderPass& renderPass, VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages);

};