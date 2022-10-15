#pragma once

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include <vector>
#include <utility>
#include <string>

class VulkanEngine;
class Camera;
class Mesh;
class Material;
class MeshManager;
class TextureManager;
class PipelineBuilder;
class Device;
class UploadContext;

struct RenderObject {
    Mesh* mesh;
    Material* material;
    glm::mat4 transformMatrix;
};

struct GPUSceneData {
    glm::vec4 fogColor;
    glm::vec4 fogDistance;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;
};

struct GPUObjectData {
    glm::mat4 modelMatrix;
};

typedef std::vector<RenderObject> Renderables;

class SceneListing final {
public:
    static const std::vector<std::pair<std::string, std::function<Renderables (Camera& camera, VulkanEngine* engine)>>> scenes;

    static Renderables monkeyAndTriangles(Camera& camera, VulkanEngine* engine);
    static Renderables lostEmpire(Camera &camera, VulkanEngine* engine);
    static Renderables cubeScene(Camera &camera, VulkanEngine* engine);
    static Renderables sponza(Camera& camera, VulkanEngine* engine);
    static Renderables oldBridge(Camera& camera, VulkanEngine* engine);

private:
    static const class Device& _device;
    class MeshManager* _meshManager;
};