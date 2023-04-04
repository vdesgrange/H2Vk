#include "vk_scene_listing.h"
#include "core/camera/vk_camera.h"
#include "core/model/vk_model.h"
#include "core/model/vk_poly.h"
#include "core/model/vk_glb.h"
#include "core/vk_descriptor_builder.h"
#include "vk_engine.h"

const std::vector<std::pair<std::string, std::function<Renderables(Camera& camera, VulkanEngine* engine)>>> SceneListing::scenes = {
        {"None", SceneListing::empty},
        {"Spheres", SceneListing::spheres},
        {"Damaged helmet", SceneListing::damagedHelmet},
};

Renderables SceneListing::empty(Camera& camera, VulkanEngine* engine) {
    return {};
}

Renderables SceneListing::spheres(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    // === Init camera ===
    camera.inverse(false);
    camera.set_position({ 0.f, 0.0f, 5.f });
    camera.set_perspective(70.f, (float)engine->_window->_windowExtent.width /(float)engine->_window->_windowExtent.height, 0.1f, 200.0f);
    camera.set_type(Camera::Type::look_at);
    camera.set_speed(10.0f);

    // === Init shader materials ===
    std::vector<PushConstant> constants {
            {sizeof(glm::mat4), ShaderType::VERTEX},
            {sizeof(Materials::Properties), ShaderType::FRAGMENT}
    };

    std::vector<std::pair<ShaderType, const char*>> pbr_modules {
            {ShaderType::VERTEX, "../src/shaders/pbr/pbr_ibp.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/pbr/pbr_ibl_cube.frag.spv"},
    };

    std::vector<VkDescriptorSetLayout> setLayouts = {engine->_descriptorSetLayouts.environment, engine->_descriptorSetLayouts.matrices};
    engine->_materialManager->_pipelineBuilder = engine->_pipelineBuilder.get();
    engine->_materialManager->create_material("pbrMaterial", setLayouts, constants, pbr_modules);

    // === Add entities ===
    engine->_lightingManager->clear_entities();
    engine->_lightingManager->add_entity("light", std::make_shared<Light>(Light::POINT, glm::vec4(0.f, 0.f, 0.f, 0.f), glm::vec4(1.f)));

    for (int x = 0; x <= 6; x++) {
        for (int y = 0; y <= 6; y++) {
            float ratio_x = float(x) / 6.0f;
            float ratio_y = float(y) / 6.0f;
            PBRProperties gold = {{1.0f,  0.765557f, 0.336057f, 1.0f}, ratio_y * 1.0f, ratio_x * 1.0f, 1.0f};
            std::string name = "sphere_" + std::to_string(x) + "_" + std::to_string(y);
            std::shared_ptr<Model> sphereModel = ModelPOLY::create_uv_sphere(engine->_device.get(), {0.0f, 0.0f, -5.0f}, 1.0f, 32, 32, {1.0f,1.0f,  1.0f}, gold);

            engine->_meshManager->upload_mesh(*sphereModel);
            engine->_meshManager->add_entity(name, std::static_pointer_cast<Entity>(sphereModel));

            RenderObject sphere;
            sphere.model = engine->_meshManager->get_model(name);
            sphere.material = engine->_materialManager->get_material("pbrMaterial");
            glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x - 3, y - 3, 0));
            glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.5, 0.5, 0.5));
            sphere.transformMatrix = translation * scale;
            renderables.push_back(sphere);
        }
    }

    return renderables;
}

Renderables SceneListing::damagedHelmet(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    // === Init camera ===
    camera.inverse(true);
    camera.set_position({ 0.0f, 0.0f, 3.0f }); // Re-initialize position after scene change = camera jumping.
    camera.set_perspective(70.f,  (float)engine->_window->_windowExtent.width /(float)engine->_window->_windowExtent.height, 0.1f, 200.0f);  // 1700.f / 1200.f
    camera.set_type(Camera::Type::look_at);
    camera.set_speed(10.0f);

    // === Add entities ===
    engine->_lightingManager->clear_entities();
    engine->_lightingManager->add_entity("light", std::make_shared<Light>(Light::POINT, glm::vec4(0.f, 0.f, 0.f, 0.f), glm::vec4(1.f)));

    std::shared_ptr<ModelGLB> helmetModel = std::make_shared<ModelGLB>(engine->_device.get());
    helmetModel->load_model(*engine->_device, engine->_uploadContext, "../assets/damaged_helmet/gltf_bin/DamagedHelmet.glb");
    engine->_meshManager->upload_mesh(*helmetModel);
    engine->_meshManager->add_entity("helmet", std::static_pointer_cast<Entity>(helmetModel));

    // === Init shader materials ===
    std::vector<PushConstant> constants {
            {sizeof(glm::mat4), ShaderType::VERTEX},
    };

    std::vector<std::pair<ShaderType, const char*>> pbr_modules {
            {ShaderType::VERTEX, "../src/shaders/pbr/pbr_tex.vert.spv"},
            {ShaderType::FRAGMENT, "../src/shaders/pbr/pbr_tex_cube.frag.spv"},
    };

    VkDescriptorSetLayout textures{};
    helmetModel->setup_descriptors(*engine->_layoutCache, *engine->_allocator, textures);
    std::vector<VkDescriptorSetLayout> setLayouts = {engine->_descriptorSetLayouts.environment, engine->_descriptorSetLayouts.matrices, textures};
    engine->_materialManager->_pipelineBuilder = engine->_pipelineBuilder.get(); // todo move pipelineBuilder variable to create_material
    engine->_materialManager->create_material("pbrTextureMaterial", setLayouts, constants, pbr_modules);

    // == Init scene ==
    RenderObject helmet;
    helmet.model = engine->_meshManager->get_model("helmet");
    helmet.material = engine->_materialManager->get_material("pbrTextureMaterial");
    helmet.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(helmet);

    return renderables;
}