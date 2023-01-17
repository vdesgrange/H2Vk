#include "vk_scene.h"

void Scene::load_scene(int sceneIndex, Camera& camera) {
    if (sceneIndex == _sceneIndex) {
        return;
    }

    auto renderables = SceneListing::scenes[sceneIndex].second(camera, &_engine);
    _sceneIndex = sceneIndex;
    _renderables = renderables;
}