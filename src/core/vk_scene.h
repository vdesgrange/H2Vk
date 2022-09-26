#pragma once

#include <vector>

class SceneListing;
class MeshManager;
class PipelineBuilder;
class Camera;
struct RenderObject;

typedef std::vector<RenderObject> Renderables;

class Scene final {
public:
    int _sceneIndex;
    Renderables _renderables;

    Scene(MeshManager& meshManager, PipelineBuilder& pipelineBuilder);
    ~Scene();

    void load_scene(int sceneIndex, Camera& camera);

private:
    MeshManager& _meshManager;
    PipelineBuilder& _pipelineBuilder;
};