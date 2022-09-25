#pragma once

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include <vector>
#include <utility>
#include <string>

class Camera;
class Mesh;
class Material;
class MeshManager;
class PipelineBuilder;

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
    static const std::vector<std::pair<std::string, std::function<Renderables (Camera& camera, MeshManager* meshManager, PipelineBuilder* pipelineBuilder)>>> scenes;

    SceneListing(MeshManager* meshManager, PipelineBuilder* pipelineBuilder) : _meshManager(meshManager), _pipelineBuilder(pipelineBuilder) {};
    ~SceneListing();

    static Renderables monkeyAndTriangles(Camera& camera, MeshManager* meshManager, PipelineBuilder* pipelineBuilder);
    static Renderables lostEmpire(Camera &camera, MeshManager* meshManager, PipelineBuilder* pipelineBuilder);

private:
    class MeshManager* _meshManager;
    class PipelineBuilder* _pipelineBuilder;
};