#include "vk_scene_listing.h"
#include "vk_mesh_manager.h"
#include "vk_pipeline.h"
#include "vk_camera.h"
#include "vk_mesh.h"
#include "vk_texture.h"
#include "vk_helpers.h"

const std::vector<std::pair<std::string, std::function<Renderables(Camera& camera, MeshManager* meshManager, TextureManager* textureManager, PipelineBuilder* pipelineBuilder)>>> SceneListing::scenes = {
        {"Monkey and triangles", SceneListing::monkeyAndTriangles},
        {"Lost empire", SceneListing::lostEmpire}
};

Renderables SceneListing::monkeyAndTriangles(Camera& camera, MeshManager* meshManager, TextureManager* textureManager, PipelineBuilder* pipelineBuilder) {
    Renderables renderables{};

    camera.set_flip_y(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    // Upload mesh
    Mesh mesh{};
    Mesh objMesh{};

    mesh._vertices.resize(3);

    mesh._vertices[0].position = { 1.f, 1.f, 0.0f };
    mesh._vertices[1].position = {-1.f, 1.f, 0.0f };
    mesh._vertices[2].position = { 0.f,-1.f, 0.0f };

    mesh._vertices[0].color = { 0.f, 1.f, 0.0f }; //pure green
    mesh._vertices[1].color = { 0.f, 1.f, 0.0f }; //pure green
    mesh._vertices[2].color = { 0.f, 1.f, 0.0f }; //pure green

    objMesh.load_from_obj("../assets/monkey_smooth.obj");
    meshManager->upload_mesh(mesh);
    meshManager->upload_mesh(objMesh);

    meshManager->_meshes["monkey"] = objMesh;
    meshManager->_meshes["triangle"] = mesh;

    // Shaders - to do

    // From init_scene
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

Renderables SceneListing::lostEmpire(Camera& camera, MeshManager* meshManager, TextureManager* textureManager, PipelineBuilder* pipelineBuilder) {
    Renderables renderables{};

    camera.set_flip_y(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    // Upload mesh
    Mesh lostEmpire{};
    lostEmpire.load_from_obj("../assets/lost_empire.obj");
    meshManager->upload_mesh(lostEmpire);
    meshManager->_meshes["empire"] = lostEmpire;

    // Load texture -> important : rajouter hashage pour ne pas dupliquer les textures
    // textureManager->load_texture("../assets/lost_empire-RGBA.png", "empire_diffuse");

    // Shaders - to do

    // From init_scene
    RenderObject map;
    map.mesh = meshManager->get_mesh("empire");
    map.material = pipelineBuilder->get_material("texturedMesh");
    map.transformMatrix = glm::translate(glm::mat4(1.f), glm::vec3{ 5,-10,0 });
    renderables.push_back(map);

    return renderables;
}

