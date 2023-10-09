/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_scene_listing.h"
#include "components/camera/vk_camera.h"
#include "components/model/vk_model.h"
#include "components/model/vk_poly.h"
#include "components/model/vk_gltf.h"
#include "components/model/vk_gltf2.h"
#include "components/model/vk_glb.h"
#include "core/vk_descriptor_builder.h"
#include "vk_engine.h"
#include <iostream>
#include <chrono>

const std::vector<std::pair<std::string, std::function<Renderables(Camera& camera, VulkanEngine* engine)>>> SceneListing::scenes = {
        {"None", SceneListing::empty},
        {"Spheres", SceneListing::spheres},
        {"Damaged helmet", SceneListing::damagedHelmet},
        {"Sponza", SceneListing::sponza},
        {"Field", SceneListing::field},
};

Renderables SceneListing::empty(Camera& camera, VulkanEngine* engine) {
    return {};
}

Renderables SceneListing::spheres(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    // === Init camera ===
    camera.inverse(false);
    camera.set_position({ 0.0f, 0.0f, 10.0f });
    camera.set_perspective(70.f, (float)engine->_window->_windowExtent.width /(float)engine->_window->_windowExtent.height, 0.1f, 50.0f);
    camera.set_type(Camera::Type::look_at);
    camera.set_speed(10.0f);

    // === Add entities ===
    engine->_lightingManager->clear_entities();
    engine->_lightingManager->add_entity("sun", std::make_shared<Light>(glm::vec4(0.0f, 0.0f, 0.0f, 0.f),glm::vec4(1.f)));

    Materials blank = {{1.0f, 1.0f, 1.0, 1.0f}, 0.0f,  1.0f, 0.0f};

    std::shared_ptr<Model> floorModel = ModelPOLY::create_plane(engine->_device.get(), engine->_uploadContext, {-4.0f, 4.0f, -6.0f}, {4.0f, 4.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, blank);
    engine->_meshManager->upload_mesh(*floorModel);
    engine->_meshManager->add_entity("floor", std::static_pointer_cast<Entity>(floorModel));

    std::shared_ptr<Model> wallModel = ModelPOLY::create_plane(engine->_device.get(), engine->_uploadContext, {-4.0f, -4.0f, -6.0f}, {4.0f, 4.0f, -6.0f}, {1.0f, 1.0f, 1.0f}, blank);
    engine->_meshManager->upload_mesh(*wallModel);
    engine->_meshManager->add_entity("wall", std::static_pointer_cast<Entity>(wallModel));


    // === Init shader materials ===
    unsigned char pixels[] = {0, 0, 0, 1};
    Texture emptyTexture{};
    emptyTexture.load_image_from_buffer(*engine->_device, engine->_uploadContext, pixels, 4, VK_FORMAT_R8G8B8A8_UNORM, 1, 1); // ctx?

    std::vector<PushConstant> constants {
            {sizeof(glm::mat4), ShaderType::VERTEX},
            {sizeof(Materials::Factors), ShaderType::FRAGMENT}
    };

    std::vector<std::pair<ShaderType, const char*>> modules {
            {ShaderType::VERTEX, "../src/shaders/pbr/pbr_ibl.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/pbr/pbr_ibl.frag.spv"},
    };

    floorModel->setup_descriptors(*engine->_layoutCache, *engine->_allocator, engine->_descriptorSetLayouts.textures, emptyTexture);
    std::vector<VkDescriptorSetLayout> setLayouts = {engine->_descriptorSetLayouts.environment, engine->_descriptorSetLayouts.matrices, engine->_descriptorSetLayouts.textures};
    engine->_materialManager->_pipelineBuilder = engine->_pipelineBuilder.get(); // todo move pipelineBuilder variable to create_material
    engine->_materialManager->create_material("pbrMaterial", setLayouts, constants, modules);

    wallModel->setup_descriptors(*engine->_layoutCache, *engine->_allocator, engine->_descriptorSetLayouts.textures, emptyTexture);
    engine->_materialManager->_pipelineBuilder = engine->_pipelineBuilder.get(); // todo move pipelineBuilder variable to create_material
    engine->_materialManager->create_material("pbrMaterial2", setLayouts, constants, modules);


    std::vector<VkDescriptorSetLayout> shadowSetLayouts = {engine->_descriptorSetLayouts.environment, engine->_descriptorSetLayouts.matrices};
    engine->_materialManager->create_material("shadowScene", shadowSetLayouts, constants, modules);

    RenderObject floor;
    floor.model = engine->_meshManager->get_model("floor");
    floor.material = engine->_materialManager->get_material("pbrMaterial");
    floor.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(floor);

    RenderObject wall;
    wall.model = engine->_meshManager->get_model("wall");
    wall.material = engine->_materialManager->get_material("pbrMaterial2");
    wall.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(wall);

    for (int x = 0; x <= 6; x++) {
        for (int y = 0; y <= 6; y++) {
            float ratio_x = float(x) / 6.0f;
            float ratio_y = float(y) / 6.0f;
            Materials gold = {{1.0f,  0.765557f, 0.336057f, 1.0f}, ratio_y * 1.0f, ratio_x * 1.0f, 1.0f};
            std::string name = "sphere_" + std::to_string(x) + "_" + std::to_string(y);
            std::shared_ptr<Model> sphereModel = ModelPOLY::create_uv_sphere(engine->_device.get(), engine->_uploadContext, {0.0f, 0.0f, -5.0f}, 1.0f, 32, 32, {1.0f,1.0f,  1.0f}, gold);

            sphereModel->setup_descriptors(*engine->_layoutCache, *engine->_allocator, engine->_descriptorSetLayouts.textures, emptyTexture);
            engine->_materialManager->_pipelineBuilder = engine->_pipelineBuilder.get(); // todo move pipelineBuilder variable to create_material
            engine->_materialManager->create_material("spherePbrMaterial_" + std::to_string(7 * x + y), setLayouts, constants, modules);

            engine->_meshManager->upload_mesh(*sphereModel);
            engine->_meshManager->add_entity(name, std::static_pointer_cast<Entity>(sphereModel));

            RenderObject sphere;
            sphere.model = engine->_meshManager->get_model(name);
            sphere.material = engine->_materialManager->get_material("spherePbrMaterial_" + std::to_string(7 * x + y));
            glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x - 3, y - 3, 0));
            glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.5, 0.5, 0.5));
            sphere.transformMatrix = translation * scale;
            renderables.push_back(sphere);
        }
    }

    emptyTexture.destroy(*engine->_device); // todo - generate problems when clean-up 
    return renderables;
}

Renderables SceneListing::damagedHelmet(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    // === Empty texture ===
    unsigned char pixels[] = {0, 0, 0, 1};
    Texture emptyTexture{};
    emptyTexture.load_image_from_buffer(*engine->_device, engine->_uploadContext, pixels, 4, VK_FORMAT_R8G8B8A8_UNORM, 1, 1); // ctx?

    // === Init camera ===
    camera.inverse(true);
    camera.set_position({ 0.0f, 0.0f, 3.0f }); // Re-initialize position after scene change = camera jumping.
    camera.set_perspective(70.f,  (float)engine->_window->_windowExtent.width /(float)engine->_window->_windowExtent.height, 0.1f, 200.0f);  // 1700.f / 1200.f
    camera.set_type(Camera::Type::look_at);
    camera.set_speed(10.0f);

    // === Add entities ===
    engine->_lightingManager->clear_entities();
    engine->_lightingManager->add_entity("sun", std::make_shared<Light>(glm::vec4(0.f, 0.f, 0.f, 0.f),  glm::vec4(1.f)));

    std::shared_ptr<ModelGLTF2> helmetModel = std::make_shared<ModelGLTF2>(engine->_device.get());
    helmetModel->load_model(*engine->_device, engine->_uploadContext, "../assets/damaged_helmet/gltf/DamagedHelmet.gltf");
    engine->_meshManager->upload_mesh(*helmetModel);
    engine->_meshManager->add_entity("helmet", std::static_pointer_cast<Entity>(helmetModel));

    // === Init shader materials ===
    std::vector<PushConstant> constants {
            {sizeof(glm::mat4), ShaderType::VERTEX},
            {sizeof(Materials::Factors), ShaderType::FRAGMENT}
    };

    std::vector<std::pair<ShaderType, const char*>> pbr_modules {
            {ShaderType::VERTEX, "../src/shaders/pbr/pbr_ibl_tex.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/pbr/pbr_ibl_tex.frag.spv"},
    };

    // VkDescriptorSetLayout textures{}; // todo : stop duplicate with _scene->setup_texture_descriptors
    helmetModel->setup_descriptors(*engine->_layoutCache, *engine->_allocator, engine->_descriptorSetLayouts.textures, emptyTexture);
    std::vector<VkDescriptorSetLayout> setLayouts = {engine->_descriptorSetLayouts.environment, engine->_descriptorSetLayouts.matrices, engine->_descriptorSetLayouts.textures};
    engine->_materialManager->_pipelineBuilder = engine->_pipelineBuilder.get(); // todo move pipelineBuilder variable to create_material
    engine->_materialManager->create_material("pbrTextureMaterial", setLayouts, constants, pbr_modules);

    // == Init scene ==
    RenderObject helmet;
    helmet.model = engine->_meshManager->get_model("helmet");
    helmet.material = engine->_materialManager->get_material("pbrTextureMaterial");
    helmet.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(helmet);

    emptyTexture.destroy(*engine->_device); // todo - generate problems when clean-up 

    return renderables;
}

Renderables SceneListing::sponza(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    // === Empty texture ===
    unsigned char pixels[] = {0, 0, 0, 1};
    Texture emptyTexture{};
    emptyTexture.load_image_from_buffer(*engine->_device, engine->_uploadContext, pixels, 4, VK_FORMAT_R8G8B8A8_UNORM, 1, 1); // ctx?

    // === Init camera ===
    camera.inverse(true);
    camera.set_position({ 0.0f, -5.0f, 0.0f }); // Re-initialize position after scene change = camera jumping.
    camera.set_perspective(70.f,  (float)engine->_window->_windowExtent.width /(float)engine->_window->_windowExtent.height, 0.1f, 200.0f);
    camera.set_type(Camera::Type::pov);
    camera.set_speed(10.0f);

    // === Add entities ===
    engine->_lightingManager->clear_entities();
    engine->_lightingManager->add_entity("sun", std::make_shared<Light>(glm::vec4(0.f, 0.f, 0.f, 0.f),  glm::vec4(1.f)));

    std::shared_ptr<ModelGLTF2> sponzaModel = std::make_shared<ModelGLTF2>(engine->_device.get());
    sponzaModel->load_model(*engine->_device, engine->_uploadContext, "../assets/glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf");

    engine->_meshManager->upload_mesh(*sponzaModel);
    engine->_meshManager->add_entity("sponza", std::static_pointer_cast<Entity>(sponzaModel));

    // === Init shader materials ===
    std::vector<PushConstant> constants {
            {sizeof(glm::mat4), ShaderType::VERTEX},
            {sizeof(Materials::Factors), ShaderType::FRAGMENT}
    };

    std::vector<std::pair<ShaderType, const char*>> pbr_modules {
            {ShaderType::VERTEX, "../src/shaders/pbr/pbr_ibl_tex.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/pbr/pbr_ibl_tex.frag.spv"},
    };

    VkDescriptorSetLayout textures{};
    sponzaModel->setup_descriptors(*engine->_layoutCache, *engine->_allocator, textures, emptyTexture);
    std::vector<VkDescriptorSetLayout> setLayouts = {engine->_descriptorSetLayouts.environment, engine->_descriptorSetLayouts.matrices, textures};
    engine->_materialManager->_pipelineBuilder = engine->_pipelineBuilder.get();
    engine->_materialManager->create_material("sponzaMaterial", setLayouts, constants, pbr_modules);

    // == Init scene ==
    RenderObject sponza;
    sponza.model = engine->_meshManager->get_model("sponza");
    sponza.material = engine->_materialManager->get_material("sponzaMaterial");
    sponza.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(sponza);

    emptyTexture.destroy(*engine->_device); // todo - generate problems when clean-up 

    return renderables;
}

Renderables SceneListing::field(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    // === Empty texture ===
    unsigned char pixels[] = {0, 0, 0, 1};
    Texture emptyTexture{};
    emptyTexture.load_image_from_buffer(*engine->_device, engine->_uploadContext, pixels, 4, VK_FORMAT_R8G8B8A8_UNORM, 1, 1); // ctx?

    // === Init camera ===
    camera.inverse(true);
    camera.set_position({ 0.0f, -5.0f, 0.0f }); // Re-initialize position after scene change = camera jumping.
    camera.set_perspective(70.f,  (float)engine->_window->_windowExtent.width /(float)engine->_window->_windowExtent.height, 0.1f, 70.0f);
    camera.set_type(Camera::Type::pov);
    camera.set_speed(10.0f);

    // === Add entities ===
    engine->_lightingManager->clear_entities();
    engine->_lightingManager->add_entity("sun", std::make_shared<Light>(glm::vec4(0.f, 0.f, 0.f, 0.f),  glm::vec4(1.f)));

    std::shared_ptr<ModelGLTF2> treeModel = std::make_shared<ModelGLTF2>(engine->_device.get());
    treeModel->load_model(*engine->_device, engine->_uploadContext, "../assets/field/oaktree.gltf");
    engine->_meshManager->upload_mesh(*treeModel);
    engine->_meshManager->add_entity("tree", std::static_pointer_cast<Entity>(treeModel));

    std::shared_ptr<ModelGLTF2> fieldModel = std::make_shared<ModelGLTF2>(engine->_device.get());
    fieldModel->load_model(*engine->_device, engine->_uploadContext, "../assets/field/terrain_gridlines.gltf");
    engine->_meshManager->upload_mesh(*fieldModel);
    engine->_meshManager->add_entity("field", std::static_pointer_cast<Entity>(fieldModel));

    // === Init shader materials ===
    std::vector<PushConstant> constants {
            {sizeof(glm::mat4), ShaderType::VERTEX},
            {sizeof(Materials::Factors), ShaderType::FRAGMENT}
    };

   std::vector<std::pair<ShaderType, const char*>> modules {
           {ShaderType::VERTEX, "../src/shaders/shadow_map/depth_debug_scene.vert.spv"},
           {ShaderType::FRAGMENT, "../src/shaders/shadow_map/scene_debug.frag.spv"},
   };


    treeModel->setup_descriptors(*engine->_layoutCache, *engine->_allocator, engine->_descriptorSetLayouts.textures, emptyTexture);
    std::vector<VkDescriptorSetLayout> setLayouts = {engine->_descriptorSetLayouts.environment, engine->_descriptorSetLayouts.matrices, engine->_descriptorSetLayouts.textures};
    engine->_materialManager->_pipelineBuilder = engine->_pipelineBuilder.get();
    engine->_materialManager->create_material("treeMaterial", setLayouts, constants, modules);

    fieldModel->setup_descriptors(*engine->_layoutCache, *engine->_allocator, engine->_descriptorSetLayouts.textures, emptyTexture);
    engine->_materialManager->_pipelineBuilder = engine->_pipelineBuilder.get();
    engine->_materialManager->create_material("fieldMaterial", setLayouts, constants, modules);


    // == Init scene ==
    RenderObject tree;
    tree.model = engine->_meshManager->get_model("tree");
    tree.material = engine->_materialManager->get_material("treeMaterial");
    glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(1.25f, -0.25f, 1.25f));
    glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(1.0f));
    tree.transformMatrix = translation * scale;
    renderables.push_back(tree);

    RenderObject field;
    field.model = engine->_meshManager->get_model("field");
    field.material = engine->_materialManager->get_material("fieldMaterial");
    field.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(field);

    emptyTexture.destroy(*engine->_device); // todo - generate problems when clean-up 

    return renderables;
}
