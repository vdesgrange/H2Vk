#include "vk_scene.h"
#include "vk_scene_listing.h"
#include "core/vk_camera.h"

Scene::Scene(VulkanEngine& engine) : _engine(engine) {}

void Scene::load_scene(int sceneIndex, Camera& camera) {
    if (sceneIndex == _sceneIndex) {
        return;
    }

    auto renderables = SceneListing::scenes[sceneIndex].second(camera, &_engine);
    _sceneIndex = sceneIndex;
    _renderables = renderables;
}