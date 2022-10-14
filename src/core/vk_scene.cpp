#include "vk_scene.h"
#include "vk_scene_listing.h"
#include "vk_camera.h"
#include "vk_mesh_manager.h"
#include "vk_texture.h"
#include "vk_pipeline.h"

Scene::Scene(VulkanEngine& engine, MeshManager& meshManager, TextureManager& textureManager, PipelineBuilder& pipelineBuilder) :
 _engine(engine), _meshManager(meshManager), _textureManager(textureManager), _pipelineBuilder(pipelineBuilder) {
}

void Scene::load_scene(int sceneIndex, Camera& camera) {
    if (sceneIndex == _sceneIndex) {
        return;
    }

    auto renderables = SceneListing::scenes[sceneIndex].second(camera, &_engine);
    _sceneIndex = sceneIndex;
    _renderables = renderables;
}