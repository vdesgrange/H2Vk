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
    MaterialManager(const Device* _device, PipelineBuilder* pipelineBuilder);
    ~MaterialManager();

    std::shared_ptr<Material> get_material(const std::string &name);
    std::shared_ptr<Material> create_material(std::string name, std::vector<VkDescriptorSetLayout> setLayouts, std::vector<PushConstant> constants, std::vector<std::pair<ShaderType, const char*>> shaders);

    void scene_spheres(std::vector<VkDescriptorSetLayout> setLayouts);
    void scene_damaged_helmet(std::vector<VkDescriptorSetLayout> setLayouts);

private:
    const class Device* _device;
    class PipelineBuilder* _pipelineBuilder;
};