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
    monkeyModel.load_from_obj("../assets/monkey_smooth.obj");
    for (auto& node : monkeyModel._nodes) {
        node->matrix =  glm::mat4{ 1.0f };
    }
    engine->_meshManager->upload_mesh(monkeyModel);
    engine->_meshManager->_models["monkey"] = monkeyModel;

    RenderObject monkey;
    monkey.model = engine->_meshManager->get_model("monkey");
    monkey.material = engine->_pipelineBuilder->get_material("defaultMesh");
    // monkey.transformMatrix = glm::mat4{ 1.0f };
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

    // Upload mesh
    //    Mesh mesh{};
    //    Mesh objMesh{};
    //
    //    mesh._vertices.resize(3);
    //
    //    mesh._vertices[0].position = { 1.f, 1.f, 0.0f };
    //    mesh._vertices[1].position = {-1.f, 1.f, 0.0f };
    //    mesh._vertices[2].position = { 0.f,-1.f, 0.0f };
    //
    //    mesh._vertices[0].color = { 0.f, 1.f, 0.0f }; //pure green
    //    mesh._vertices[1].color = { 0.f, 1.f, 0.0f }; //pure green
    //    mesh._vertices[2].color = { 0.f, 1.f, 0.0f }; //pure green
    //
    //    objMesh.load_from_obj("../assets/monkey_smooth.obj");
    //    engine->_meshManager->upload_mesh(mesh);
    //    engine->_meshManager->upload_mesh(objMesh);
    //
    //    engine->_meshManager->_meshes["monkey"] = objMesh;
    //    engine->_meshManager->_meshes["triangle"] = mesh;
    //
    //
    //    // From init_scene
    //    RenderObject monkey;
    //    monkey.mesh = engine->_meshManager->get_mesh("monkey");
    //    monkey.material = engine->_pipelineBuilder->get_material("defaultMesh");
    //    monkey.transformMatrix = glm::mat4{ 1.0f };
    //    renderables.push_back(monkey);
    //
    //    for (int x = -20; x <= 20; x++) {
    //        for (int y = -20; y <= 20; y++) {
    //            RenderObject tri;
    //            tri.mesh = engine->_meshManager->get_mesh("triangle");
    //            tri.material = engine->_pipelineBuilder->get_material("defaultMesh");
    //            glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
    //            glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
    //            tri.transformMatrix = translation * scale;
    //            renderables.push_back(tri);
    //        }
    //    }

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
    Mesh lostEmpire{};
    lostEmpire.load_from_obj("../assets/lost_empire.obj");
    engine->_meshManager->upload_mesh(lostEmpire);
    engine->_meshManager->_meshes["empire"] = lostEmpire;

    // Load texture -> important : rajouter hashage pour ne pas dupliquer les textures
//    engine->_textureManager->load_texture("../assets/lost_empire-RGBA.png", "empire_diffuse");
//    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);
//    VkSampler blockySampler; // add cache for sampler
//    vkCreateSampler(engine->_device->_logicalDevice, &samplerInfo, nullptr, &blockySampler);
//    engine->_samplerManager->_loadedSampler["blocky_sampler"] = blockySampler;
//
//    Material* texturedMat =	engine->_pipelineBuilder->get_material("texturedMesh");
//
//    VkDescriptorImageInfo imageBufferInfo;
//    imageBufferInfo.sampler = engine->_samplerManager->_loadedSampler["blocky_sampler"];
//    imageBufferInfo.imageView = engine->_textureManager->_loadedTextures["empire_diffuse"].imageView;
//    imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//
//    DescriptorBuilder::begin(*engine->_layoutCache, *engine->_allocator)
//            .bind_image(imageBufferInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texturedMat->textureSet, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
//            .build(texturedMat->textureSet, engine->_descriptorSetLayouts.textures, poolSizes);

//    // Shaders - to do

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

//    Model sponza = Model{};
//    sponza.load_from_gltf("../assets/sponza/NewSponza_Main_Blender_glTF.gltf");

    return renderables;
}

Renderables SceneListing::oldBridge(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.f, -6.f, -10.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);

    Model oldBridgeModel{};
    oldBridgeModel.load_from_glb(*engine, "../assets/old_brick_bridge/old_brick_bridge.glb");
    engine->_meshManager->upload_mesh(oldBridgeModel);
    engine->_meshManager->_models["oldBridge"] = oldBridgeModel;

    RenderObject oldBridge;
    oldBridge.model = engine->_meshManager->get_model("oldBridge");
    oldBridge.material = engine->_pipelineBuilder->get_material("defaultMesh");
    oldBridge.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(oldBridge);


    return renderables;
}
