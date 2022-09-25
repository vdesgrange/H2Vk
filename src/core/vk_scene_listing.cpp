#include "vk_scene_listing.h"
#include "vk_mesh_manager.h"
#include "vk_pipeline.h"
#include "vk_camera.h"

const std::vector<std::pair<std::string, std::function<Renderables(Camera& camera, MeshManager* meshManager, PipelineBuilder* pipelineBuilder)>>> SceneListing::scenes = {
        {"Monkey and triangles", SceneListing::monkeyAndTriangles},
        {"Lost empire", SceneListing::lostEmpire}
};

Renderables SceneListing::monkeyAndTriangles(Camera& camera, MeshManager* meshManager, PipelineBuilder* pipelineBuilder) {
    Renderables renderables{};

    camera.set_flip_y(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    RenderObject monkey;
    monkey.mesh = meshManager->get_mesh("monkey");
    monkey.material = pipelineBuilder->get_material("defaultMesh");
    monkey.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(monkey);

    for (int x = -20; x <= 20; x++) {
        for (int y = -20; y <= 20; y++) {
            RenderObject tri;
            tri.mesh = meshManager->get_mesh("triangle");
            tri.material = pipelineBuilder->get_material("defaultMesh");
            glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
            glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
            tri.transformMatrix = translation * scale;
            renderables.push_back(tri);
        }
    }

    return renderables;
}

Renderables SceneListing::lostEmpire(Camera& camera, MeshManager* meshManager, PipelineBuilder* pipelineBuilder) {
    Renderables renderables{};

    camera.set_flip_y(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    RenderObject map;
    map.mesh = meshManager->get_mesh("empire");
    map.material = pipelineBuilder->get_material("texturedMesh");
    map.transformMatrix = glm::translate(glm::mat4(1.f), glm::vec3{ 5,-10,0 });
    renderables.push_back(map);

    return renderables;
}

