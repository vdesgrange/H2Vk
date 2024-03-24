/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_material_manager.h"
#include "core/vk_device.h"
#include "core/vk_pipeline.h"
#include "components/model/vk_pbr_material.h"

MaterialManager::MaterialManager(const Device* device, PipelineBuilder* pipelineBuilder) : _device(device), _pipelineBuilder(pipelineBuilder) {}

MaterialManager::~MaterialManager() {

//    for (const auto& item : this->_entities) {
//        std::shared_ptr<Material> material = std::static_pointer_cast<Material>(item.second);
//
//        if (material->pipeline) {
//            vkDestroyPipeline(_device->_logicalDevice, material->pipeline, nullptr);
//            material->pipeline = VK_NULL_HANDLE;
//        }
//        if (material->pipelineLayout) {
//            vkDestroyPipelineLayout(_device->_logicalDevice, material->pipelineLayout, nullptr);
//            material->pipelineLayout = VK_NULL_HANDLE;
//        }
//    }
   _entities.clear();
   _discarded.clear();
}

std::shared_ptr<Material> MaterialManager::get_material(const std::string &name) {
    return std::static_pointer_cast<Material>(this->get_entity(name));
}

std::shared_ptr<Material> MaterialManager::create_material(std::string name, std::vector<VkDescriptorSetLayout> setLayouts, std::vector<PushConstant> constants, std::vector<std::pair<ShaderType, const char*>> shaders, std::unordered_map<ShaderType, VkSpecializationInfo> shaderSpecialization) {

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

    std::vector<std::tuple<VkShaderStageFlagBits, const char*, VkSpecializationInfo>> modules {};
    modules.reserve(shaders.size());
    for (const auto& it: shaders) {
        VkSpecializationInfo third = shaderSpecialization[it.first];
        modules.emplace_back(std::make_tuple(Shader::get_shader_stage(it.first), it.second, third));
    }

    std::shared_ptr<ShaderEffect> effect = _pipelineBuilder->build_effect(setLayouts, pushConstants, modules);
    std::shared_ptr<ShaderPass> pass = _pipelineBuilder->build_pass(effect);

    if (this->_entities.count(name) != 0) {
        this->_discarded.emplace(name, std::move(this->get_entity(name)));
    }

    this->add_entity(name, pass);

    for (auto& shader : effect->shaderStages) {
        vkDestroyShaderModule(_device->_logicalDevice, shader.shaderModule, nullptr);
    }

    return pass;
}

std::shared_ptr<Material> MaterialManager::create_material(PipelineBuilder& pipelineBuilder, std::string name, std::vector<VkDescriptorSetLayout> setLayouts, std::vector<PushConstant> constants, std::vector<std::pair<ShaderType, const char*>> shaders, std::unordered_map<ShaderType, VkSpecializationInfo> shaderSpecialization) {
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

    std::vector<std::tuple<VkShaderStageFlagBits, const char*, VkSpecializationInfo>> modules {};
    modules.reserve(shaders.size());
    for (const auto& it: shaders) {
        VkSpecializationInfo third = shaderSpecialization[it.first];
        modules.emplace_back(std::make_tuple(Shader::get_shader_stage(it.first), it.second, third));
    }

    std::shared_ptr<ShaderEffect> effect = pipelineBuilder.build_effect(setLayouts, pushConstants, modules);
    std::shared_ptr<ShaderPass> pass = pipelineBuilder.build_pass(effect);

    if (this->_entities.count(name) != 0) {
        this->_discarded.emplace(name, std::move(this->get_entity(name)));
    }

    this->add_entity(name, pass);

    for (auto& shader : effect->shaderStages) {
        vkDestroyShaderModule(_device->_logicalDevice, shader.shaderModule, nullptr);
    }

    return pass;
}