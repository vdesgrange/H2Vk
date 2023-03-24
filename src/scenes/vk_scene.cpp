#include "vk_scene.h"

void Scene::load_scene(int sceneIndex, Camera& camera) {
    if (sceneIndex == _sceneIndex) {
        return;
    }

    _renderables.clear();
    auto renderables = SceneListing::scenes[sceneIndex].second(camera, &_engine);
    _sceneIndex = sceneIndex;
    _renderables = renderables;
}

void Scene::setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout) {
    for (auto &renderable: this->_renderables) {
        renderable.model->setup_descriptors(layoutCache, allocator, setLayout);
    }
}