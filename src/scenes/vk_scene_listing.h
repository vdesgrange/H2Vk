/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "components/model/vk_model.h"
#include "core/utilities/vk_types.h"

#include <vector>
#include <utility>
#include <string>
#include <memory>

class VulkanEngine;
class Camera;
class Model;
class MeshManager;
class Device;

class SceneListing final {
public:
    static const std::vector<std::pair<std::string, std::function<Renderables (Camera& camera, VulkanEngine* engine)>>> scenes;

    static Renderables empty(Camera& camera, VulkanEngine* engine);
    static Renderables spheres(Camera& camera, VulkanEngine* engine);
    static Renderables damagedHelmet(Camera& camera, VulkanEngine* engine);
    static Renderables sponza(Camera& camera, VulkanEngine* engine);

private:
    static const class Device& _device;
    class MeshManager* _meshManager;
};