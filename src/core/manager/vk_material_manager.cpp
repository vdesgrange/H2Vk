#include "vk_material_manager.h"
#include "core/vk_device.h"
#include "core/vk_pipeline.h"
#include "core/model/vk_pbr_material.h"

MaterialManager::MaterialManager(const Device* device, PipelineBuilder* pipelineBuilder) : _device(device), _pipelineBuilder(pipelineBuilder) {}

MaterialManager::~MaterialManager() {

    for (const auto& item : this->_entities) {
        std::shared_ptr<Material> material = std::static_pointer_cast<Material>(item.second);

        vkDestroyPipeline(_device->_logicalDevice, material->pipeline, nullptr);
        vkDestroyPipelineLayout(_device->_logicalDevice, material->pipelineLayout, nullptr);
    }
    _entities.clear();
}

std::shared_ptr<Material> MaterialManager::get_material(const std::string &name) {
    return std::static_pointer_cast<Material>(this->get_entity(name));
}

std::shared_ptr<Material> MaterialManager::create_material(std::string name, std::vector<VkDescriptorSetLayout> setLayouts, std::vector<PushConstant> constants, std::vector<std::pair<ShaderType, const char*>> shaders) {

    std::vector<VkPushConstantRange> pushConstants {};
    uint32_t offset = 0;
    for (const auto& p: constants) {
        VkPushConstantRange push_model;
        push_model.offset = offset;
        push_model.size = p.size;
        push_model.stageFlags = Shader::get_shader_stage(p.stage);

        pushConstants.push_back(push_model);
        offset += p.size;
    }

    std::vector<std::pair<VkShaderStageFlagBits, const char*>> modules {};
    modules.reserve(shaders.size());
    for (const auto& it: shaders) {
        modules.emplace_back(std::make_pair(Shader::get_shader_stage(it.first), it.second));
    }

    std::shared_ptr<ShaderEffect> effect = _pipelineBuilder->build_effect(setLayouts, pushConstants, modules);
    std::shared_ptr<ShaderPass> pass = _pipelineBuilder->build_pass(effect);
    this->add_entity(name, pass);

    for (auto& shader : effect->shaderStages) {
        vkDestroyShaderModule(_device->_logicalDevice, shader.shaderModule, nullptr);
    }

    return pass;
}