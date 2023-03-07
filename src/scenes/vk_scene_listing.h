#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "core/model/vk_model.h"
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

private:
    static const class Device& _device;
    class MeshManager* _meshManager;
};