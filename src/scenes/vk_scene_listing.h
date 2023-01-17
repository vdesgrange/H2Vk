#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "core/model/vk_model.h"
#include "core/vk_shaders.h"

#include <vector>
#include <utility>
#include <string>

class VulkanEngine;
class Camera;
class Model;
class MeshManager;
class Device;

struct RenderObject {
    std::shared_ptr<Model> model;
    std::shared_ptr<Material> material;
    glm::mat4 transformMatrix;
};

typedef std::vector<RenderObject> Renderables;

class SceneListing final {
public:
    static const std::vector<std::pair<std::string, std::function<Renderables (Camera& camera, VulkanEngine* engine)>>> scenes;

    static Renderables empty(Camera& camera, VulkanEngine* engine);
    static Renderables monkeyAndTriangles(Camera& camera, VulkanEngine* engine);
    static Renderables karibu(Camera& camera, VulkanEngine* engine);
    static Renderables damagedHelmet(Camera& camera, VulkanEngine* engine);

private:
    static const class Device& _device;
    class MeshManager* _meshManager;
};