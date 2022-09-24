#pragma once

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include <vector>

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
    SceneListing(MeshManager* meshManager, PipelineBuilder* pipelineBuilder) : _meshManager(meshManager), _pipelineBuilder(pipelineBuilder) {};
    ~SceneListing();

    Renderables monkeyAndTriangles(Camera& camera);
    Renderables lostEmpire(Camera &camera);

private:
    class MeshManager* _meshManager;
    class PipelineBuilder* _pipelineBuilder;
};