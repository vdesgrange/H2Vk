#pragma once

#include <vector>

class VulkanEngine;
struct RenderObject;
class Camera;

typedef std::vector<RenderObject> Renderables;

class Scene final {
public:
    int _sceneIndex;
    Renderables _renderables;

    Scene(VulkanEngine& engine);

    void load_scene(int sceneIndex, Camera& camera);

private:
    VulkanEngine& _engine;
};