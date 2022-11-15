#include "vk_scene_listing.h"
#include "vk_mesh_manager.h"
#include "vk_pipeline.h"
#include "vk_camera.h"
#include "assets/vk_mesh.h"
#include "vk_texture.h"
#include "vk_helpers.h"
#include "vk_engine.h"

const std::vector<std::pair<std::string, std::function<Renderables(Camera& camera, VulkanEngine* engine)>>> SceneListing::scenes = {
        {"Monkey and triangles", SceneListing::monkeyAndTriangles},
        {"Lost empire", SceneListing::lostEmpire},
        {"Swimming pool", SceneListing::cubeScene},
        {"Sponza", SceneListing::sponza},
        {"Old bridge", SceneListing::oldBridge},
};

Renderables SceneListing::monkeyAndTriangles(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
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
    engine->_meshManager->upload_mesh(mesh);
    engine->_meshManager->upload_mesh(objMesh);

    engine->_meshManager->_meshes["monkey"] = objMesh;
    engine->_meshManager->_meshes["triangle"] = mesh;


    // From init_scene
    RenderObject monkey;
    monkey.mesh = engine->_meshManager->get_mesh("monkey");
    monkey.material = engine->_pipelineBuilder->get_material("defaultMesh");
    monkey.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(monkey);

    for (int x = -20; x <= 20; x++) {
        for (int y = -20; y <= 20; y++) {
            RenderObject tri;
            tri.mesh = engine->_meshManager->get_mesh("triangle");
            tri.material = engine->_pipelineBuilder->get_material("defaultMesh");
            glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
            glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
            tri.transformMatrix = translation * scale;
            renderables.push_back(tri);
        }
    }

    return renderables;
}

Renderables SceneListing::lostEmpire(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    // Upload mesh
    Mesh lostEmpire{};
    lostEmpire.load_from_obj("../assets/lost_empire.obj");
    engine->_meshManager->upload_mesh(lostEmpire);
    engine->_meshManager->_meshes["empire"] = lostEmpire;

    // Load texture -> important : rajouter hashage pour ne pas dupliquer les textures
    // textureManager->load_texture("../assets/lost_empire-RGBA.png", "empire_diffuse");

    // Shaders - to do

    // From init_scene
    RenderObject map;
    map.mesh = engine->_meshManager->get_mesh("empire");
    map.material = engine->_pipelineBuilder->get_material("texturedMesh");
    map.transformMatrix = glm::translate(glm::mat4(1.f), glm::vec3{ 5,-10,0 });
    renderables.push_back(map);

    return renderables;
}

Renderables SceneListing::cubeScene(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    Mesh cube = Mesh::cube();
    engine->_meshManager->upload_mesh(cube);
    engine->_meshManager->_meshes["cube"] = cube;

    RenderObject cubeObj;
    cubeObj.mesh = engine->_meshManager->get_mesh("cube");
    cubeObj.material = engine->_pipelineBuilder->get_material("defaultMesh");
    cubeObj.transformMatrix = glm::mat4(1.f);
    renderables.push_back(cubeObj);

    return renderables;
}

Renderables SceneListing::sponza(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    Model sponza{};
    sponza.load_from_gltf(*engine, "../assets/sponza/NewSponza_Main_Blender_glTF.gltf");

    return renderables;
}

Renderables SceneListing::oldBridge(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    // Model oldBridge(*engine);
    Model oldBridge{};
    oldBridge.load_from_glb(*engine, "../assets/old_brick_bridge/old_brick_bridge.glb");
    engine->_meshManager->upload_mesh(oldBridge);
    engine->_meshManager->_models["oldBridge"] = oldBridge;

    RenderObject map;
    map.model = engine->_meshManager->get_model("oldBridge");
    map.material = engine->_pipelineBuilder->get_material("texturedMesh");
    map.transformMatrix = glm::translate(glm::mat4(1.f), glm::vec3{ 5,-10,0 });
    renderables.push_back(map);

    return renderables;
}
