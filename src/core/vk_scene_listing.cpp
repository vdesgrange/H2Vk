#include "vk_scene_listing.h"
#include "vk_mesh_manager.h"
#include "vk_pipeline.h"
#include "vk_camera.h"
#include "vk_texture.h"
#include "vk_helpers.h"
#include "vk_engine.h"
#include "vk_device.h"
#include "assets/vk_mesh.h"
#include "vk_descriptor_builder.h"

const std::vector<std::pair<std::string, std::function<Renderables(Camera& camera, VulkanEngine* engine)>>> SceneListing::scenes = {
//        {"Monkey and triangles", SceneListing::monkeyAndTriangles},
//        {"Lost empire", SceneListing::lostEmpire},
//        {"Swimming pool", SceneListing::cubeScene},
//        {"Old bridge", SceneListing::oldBridge},
        {"Karibu", SceneListing::karibu},
        {"DamagedHelmet", SceneListing::damagedHelmet},
};

Renderables SceneListing::monkeyAndTriangles(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    Model triangleModel{};
    triangleModel._verticesBuffer.resize(3);
    triangleModel._verticesBuffer[0].position = { 1.f, 1.f, 0.0f };
    triangleModel._verticesBuffer[1].position = {-1.f, 1.f, 0.0f };
    triangleModel._verticesBuffer[2].position = { 0.f,-1.f, 0.0f };

    triangleModel._verticesBuffer[0].color = { 0.f, 1.f, 0.0f }; //pure green
    triangleModel._verticesBuffer[1].color = { 0.f, 1.f, 0.0f }; //pure green
    triangleModel._verticesBuffer[2].color = { 0.f, 1.f, 0.0f }; //pure green

    triangleModel._indexesBuffer = {0, 1, 2};
    engine->_meshManager->upload_mesh(triangleModel);
    engine->_meshManager->_models["triangle"] = triangleModel;

    Model monkeyModel{};
    monkeyModel.load_from_obj("../assets/monkey/monkey_smooth.obj");
    for (auto& node : monkeyModel._nodes) {
        node->matrix =  glm::mat4{ 1.0f };
    }
    engine->_meshManager->upload_mesh(monkeyModel);
    engine->_meshManager->_models["monkey"] = monkeyModel;

    RenderObject monkey;
    monkey.model = engine->_meshManager->get_model("monkey");
    monkey.material = engine->_pipelineBuilder->get_material("defaultMesh");
    monkey.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(monkey);

    for (int x = -20; x <= 20; x++) {
        for (int y = -20; y <= 20; y++) {
            RenderObject tri;
            tri.model = engine->_meshManager->get_model("triangle");
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
    std::vector<VkDescriptorPoolSize> poolSizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
    };

    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    // Upload mesh
    Model lostEmpire{};
    lostEmpire.load_from_obj("../assets/lost_empire/lost_empire.obj");
    engine->_meshManager->upload_mesh(lostEmpire);
    engine->_meshManager->_models["empire"] = lostEmpire;

    // From init_scene
    RenderObject map;
    map.model = engine->_meshManager->get_model("empire");
    // map.mesh = engine->_meshManager->get_mesh("empire");
    map.material = engine->_pipelineBuilder->get_material("texturedMesh");
    map.transformMatrix = glm::translate(glm::mat4(1.f), glm::vec3{ 5,-10,0 });
    renderables.push_back(map);

    return renderables;
}

Renderables SceneListing::cubeScene(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

//    camera.inverse(true);
//    camera.set_position({ 0.f, -6.f, -10.f });
//    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);
//
//    Mesh cube = Mesh::cube();
//    engine->_meshManager->upload_mesh(cube);
//    engine->_meshManager->_meshes["cube"] = cube;
//
//    RenderObject cubeObj;
//    cubeObj.mesh = engine->_meshManager->get_mesh("cube");
//    cubeObj.material = engine->_pipelineBuilder->get_material("defaultMesh");
//    cubeObj.transformMatrix = glm::mat4(1.f);
//    renderables.push_back(cubeObj);

    return renderables;
}

Renderables SceneListing::oldBridge(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    Model oldBridgeModel{};
    oldBridgeModel.load_from_glb(*engine, "../assets/old_brick_bridge/old_brick_bridge.glb");
    // oldBridgeModel.load_from_gltf(*engine, "../assets/old_brick_bridge/old_brick_bridge/scene.gltf");
    engine->_meshManager->upload_mesh(oldBridgeModel);
    engine->_meshManager->_models["oldBridge"] = oldBridgeModel;

    RenderObject oldBridge;
    oldBridge.model = engine->_meshManager->get_model("oldBridge");
    oldBridge.material = engine->_pipelineBuilder->get_material("defaultMesh");
    oldBridge.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(oldBridge);


    return renderables;
}

Renderables SceneListing::karibu(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    Model karibuModel{};
    karibuModel.load_from_glb(*engine, "../assets/karibu_hippo_zanzibar/karibu_hippo_zanzibar.glb");
    engine->_meshManager->upload_mesh(karibuModel);
    engine->_meshManager->_models["karibu"] = karibuModel;

    RenderObject karibu;
    karibu.model = engine->_meshManager->get_model("karibu");
    karibu.material = engine->_pipelineBuilder->get_material("defaultMesh");
    karibu.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(karibu);


    return renderables;
}

Renderables SceneListing::damagedHelmet(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    Model helmetModel{};
    helmetModel.load_from_glb(*engine, "../assets/damaged_helmet/gltf_bin/DamagedHelmet.glb");
    engine->_meshManager->upload_mesh(helmetModel);
    engine->_meshManager->_models["helmet"] = helmetModel;

    RenderObject helmet;
    helmet.model = engine->_meshManager->get_model("helmet");
    helmet.material = engine->_pipelineBuilder->get_material("defaultMesh");
    helmet.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(helmet);

    return renderables;
}