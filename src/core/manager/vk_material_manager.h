/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <vector>
#include <iostream>
#include <unordered_map>

#include "core/manager/vk_system_manager.h"
#include "core/vk_shaders.h"

class Device;
class PipelineBuilder;

class MaterialManager : public System {
public:
    class PipelineBuilder* _pipelineBuilder;

    MaterialManager(const Device* _device, PipelineBuilder* pipelineBuilder);
    ~MaterialManager();
    MaterialManager(const MaterialManager&) = delete;
    MaterialManager(MaterialManager&&) noexcept = default;
    MaterialManager& operator=(const MaterialManager&) = delete;
    MaterialManager& operator=(MaterialManager&&) noexcept = default;

    std::shared_ptr<Material> get_material(const std::string &name);
    std::shared_ptr<Material> create_material(std::string name, std::vector<VkDescriptorSetLayout> setLayouts, std::vector<PushConstant> constants, std::vector<std::pair<ShaderType, const char*>> shaders, std::unordered_map<ShaderType, VkSpecializationInfo> shaderSpecialization = {});
    std::shared_ptr<Material> create_material(PipelineBuilder& pipelineBuilder, std::string name, std::vector<VkDescriptorSetLayout> setLayouts, std::vector<PushConstant> constants, std::vector<std::pair<ShaderType, const char*>> shaders, std::unordered_map<ShaderType, VkSpecializationInfo> shaderSpecialization = {});

private:
    const class Device* _device;
    /** @brief Maintain obsolete materials until command buffer stop running */
    std::unordered_map<std::string, std::shared_ptr<Entity>> _discarded;
};