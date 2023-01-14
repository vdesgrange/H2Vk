#pragma once

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "core/model/vk_mesh.h"
#include "vk_material.h"

#include <vector>
#include <utility>
#include <string>

class VulkanEngine;
class Camera;
class Mesh;
class Model;
class Material;
class MeshManager;
class TextureManager;
class PipelineBuilder;
class Device;
class UploadContext;

struct RenderObject {
    std::shared_ptr<Model> model;
    std::shared_ptr<Material> material;
    glm::mat4 transformMatrix;
};



struct GPUSceneData {
    glm::vec4 fogColor;
    glm::vec4 fogDistance;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;
    glm::float32 specularFactor;
};

struct GPUObjectData {
    glm::mat4 model;
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