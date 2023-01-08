#include "vk_scene_listing.h"
#include "vk_camera.h"
#include "vk_engine.h"
#include "core/model/vk_mesh.h"
#include "core/model/vk_obj.h"
#include "core/model/vk_poly.h"
#include "core/model/vk_glb.h"
#include "vk_descriptor_builder.h"

const std::vector<std::pair<std::string, std::function<Renderables(Camera& camera, VulkanEngine* engine)>>> SceneListing::scenes = {
        {"None", SceneListing::empty},
        {"Monkey and triangles", SceneListing::monkeyAndTriangles},
        {"Karibu", SceneListing::karibu},
        {"DamagedHelmet", SceneListing::damagedHelmet},
};

Renderables SceneListing::empty(Camera& camera, VulkanEngine* engine) {
    return {};
}

Renderables SceneListing::monkeyAndTriangles(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 1.f, 1.f, 1.f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);
    camera.type = Camera::Type::pov;

    std::shared_ptr<Model> triangleModel = ModelPOLY::create_plane(engine->_device.get(), {-10.f, -5.f, -10.f}, {10.f, -5.f, 10.f});
    // std::shared_ptr<Model> triangleModel = ModelPOLY::create_sphere(engine->_device.get(), {0.f, 0.f, 0.0f}, 1000.0f);
    // std::shared_ptr<Model> triangleModel = ModelPOLY::create_triangle(engine->_device.get(), {0.f, 1.f, 0.0f});
    engine->_meshManager->upload_mesh(*triangleModel);
    engine->_meshManager->_models.emplace("triangle", triangleModel);

    ModelOBJ* monkeyModel = new ModelOBJ(engine->_device.get());
    monkeyModel->load_model(*engine, "../assets/monkey/monkey_smooth.obj");
    for (auto& node : monkeyModel->_nodes) {
        node->matrix =  glm::mat4{ 1.0f };
    }
    engine->_meshManager->upload_mesh(*monkeyModel);
    engine->_meshManager->_models.emplace("monkey", std::shared_ptr<Model>(monkeyModel));

    RenderObject monkey;
    monkey.model = engine->_meshManager->get_model("monkey");
    monkey.material = engine->_pipelineBuilder->get_material("monkeyMaterial");
    monkey.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(monkey);

//    for (int x = -1; x <= 1; x++) {
//        for (int y = -1; y <= 1; y++) {
            RenderObject tri;
            tri.model = engine->_meshManager->get_model("triangle");
            tri.material = engine->_pipelineBuilder->get_material("monkeyMaterial");
            glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(0, 0, 0)); // vec3(x, 0, y)
            glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
            tri.transformMatrix = translation * scale;
            renderables.push_back(tri);
//        }
//    }

    return renderables;
}

Renderables SceneListing::karibu(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.0f, 10.0f, 0.0f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);
    camera.type = Camera::Type::axis;

    ModelGLB* karibuModel = new ModelGLB(engine->_device.get());
    karibuModel->load_model(*engine, "../assets/karibu_hippo_zanzibar/karibu_hippo_zanzibar.glb");
    engine->_meshManager->upload_mesh(*karibuModel);
    engine->_meshManager->_models.emplace("karibu", std::shared_ptr<Model>(karibuModel));

    RenderObject karibu;
    karibu.model = engine->_meshManager->get_model("karibu");
    karibu.material = engine->_pipelineBuilder->get_material("karibuMaterial");
    karibu.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(karibu);

    return renderables;
}

Renderables SceneListing::damagedHelmet(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(true);
    camera.set_position({ 0.0f, 0.0f, -3.0f });
    camera.set_perspective(70.f, 1700.f / 1200.f, 0.1f, 200.0f);
    camera.type = Camera::Type::axis;

    ModelGLB* helmetModel = new ModelGLB(engine->_device.get());
    helmetModel->load_model(*engine, "../assets/damaged_helmet/gltf_bin/DamagedHelmet.glb");
    engine->_meshManager->upload_mesh(*helmetModel);
    engine->_meshManager->_models.emplace("helmet", std::shared_ptr<Model>(helmetModel));

    RenderObject helmet;
    helmet.model = engine->_meshManager->get_model("helmet");
    helmet.material = engine->_pipelineBuilder->get_material("helmetMaterial");
    helmet.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(helmet);

    return renderables;
}