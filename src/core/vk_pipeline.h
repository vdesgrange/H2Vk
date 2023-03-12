#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "core/utilities/vk_types.h"
#include "core/manager/vk_system_manager.h"
#include "vk_shaders.h"

class Window;
class Device;
class RenderPass;

class PipelineBuilder {
public:
    enum Type { graphic, compute };

    Type _type = Type::graphic;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;

    PipelineBuilder(const Device& device, RenderPass& renderPass);

    std::shared_ptr<ShaderEffect> build_effect(std::vector<VkDescriptorSetLayout> setLayouts,  std::vector<VkPushConstantRange> pushConstants, std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> shaderModules);
    std::shared_ptr<ShaderPass> build_pass(std::shared_ptr<ShaderEffect> effect);

private:
    const class Device& _device;
    const class RenderPass& _renderPass;

    VkPipelineLayout build_layout(std::vector<VkDescriptorSetLayout> &setLayouts, std::vector<VkPushConstantRange> &pushConstants) const;
    VkPipeline build_pipeline(const Device& device, const RenderPass& renderPass, VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages);

};

class MaterialManager : public System {
public:
    MaterialManager(const Device* _device, PipelineBuilder* pipelineBuilder);
    ~MaterialManager();

    std::shared_ptr<Material> get_material(const std::string &name);

    void scene_light(std::vector<VkDescriptorSetLayout> setLayouts);
    void scene_spheres(std::vector<VkDescriptorSetLayout> setLayouts);
    void scene_damaged_helmet(std::vector<VkDescriptorSetLayout> setLayouts);

private:
    const class Device* _device;
    class PipelineBuilder* _pipelineBuilder;
};