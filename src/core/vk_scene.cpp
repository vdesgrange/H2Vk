#include "vk_scene.h"
#include "vk_scene_listing.h"
#include "vk_camera.h"
#include "vk_mesh_manager.h"
#include "vk_pipeline.h"

Scene::Scene(MeshManager& meshManager, PipelineBuilder& pipelineBuilder) : _meshManager(meshManager), _pipelineBuilder(pipelineBuilder) {

}

void Scene::load_scene(int sceneIndex, Camera& camera) {
    if (sceneIndex == _sceneIndex) {
        return;
    }

    auto renderables = SceneListing::scenes[sceneIndex].second(camera, &_meshManager, &_pipelineBuilder);
    _sceneIndex = sceneIndex;
    _renderables = renderables;
}