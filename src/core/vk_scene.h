#pragma once

#include <vector>

class VulkanEngine;
class SceneListing;
class MeshManager;
class TextureManager;
class PipelineBuilder;
class Camera;
struct RenderObject;

typedef std::vector<RenderObject> Renderables;

class Scene final {
public:
    int _sceneIndex;
    Renderables _renderables;

    Scene(VulkanEngine& engine, MeshManager& meshManager);

    void load_scene(int sceneIndex, Camera& camera);

private:
    VulkanEngine& _engine;
    MeshManager& _meshManager;
};