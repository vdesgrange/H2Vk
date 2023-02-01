#include "vk_scene_listing.h"
#include "core/vk_camera.h"
#include "vk_engine.h"
#include "core/model/vk_model.h"
#include "core/model/vk_obj.h"
#include "core/model/vk_poly.h"
#include "core/model/vk_glb.h"
#include "core/vk_descriptor_builder.h"

const std::vector<std::pair<std::string, std::function<Renderables(Camera& camera, VulkanEngine* engine)>>> SceneListing::scenes = {
        {"None", SceneListing::empty},
        {"Monkey and triangles", SceneListing::monkeyAndTriangles},
        {"DamagedHelmet", SceneListing::damagedHelmet},
};

Renderables SceneListing::empty(Camera& camera, VulkanEngine* engine) {
    return {};
}

Renderables SceneListing::monkeyAndTriangles(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(false);
    camera.set_position({ 1.f, 1.f, 1.f });
    camera.set_perspective(70.f, (float)engine->_window->_windowExtent.width /(float)engine->_window->_windowExtent.height, 0.1f, 200.0f);
    camera.type = Camera::Type::look_at;

    std::shared_ptr<Model> lightModel = ModelPOLY::create_uv_sphere(engine->_device.get(), {0.f, 0.f, 0.0f}, 0.1f);
    engine->_meshManager->upload_mesh(*lightModel);
    engine->_meshManager->_models.emplace("light", lightModel);

    RenderObject light;
    light.model = engine->_meshManager->get_model("light");
    light.material = engine->_pipelineBuilder->get_material("light");
    light.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(light);

    std::shared_ptr<Model> planeModel = ModelPOLY::create_plane(engine->_device.get(), {-10.0f, 1.0f, -10.0f}, {10.0f, 1.0f, 10.0f});
    engine->_meshManager->upload_mesh(*planeModel);
    engine->_meshManager->_models.emplace("plane", planeModel);

    RenderObject plane;
    plane.model = engine->_meshManager->get_model("plane");
    plane.material = engine->_pipelineBuilder->get_material("monkeyMaterial");
    plane.transformMatrix = glm::mat4{ 1.0f };
    renderables.push_back(plane);


    std::shared_ptr<ModelOBJ> monkeyModel = std::make_shared<ModelOBJ>(engine->_device.get());
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

    std::shared_ptr<Model> triangleModel = ModelPOLY::create_triangle(engine->_device.get(), {0.0f, 1.0f, 0.0f});
    engine->_meshManager->upload_mesh(*triangleModel);
    engine->_meshManager->_models.emplace("triangle", triangleModel);

    for (int x = -5; x <= 5; x++) {
        for (int y = -5; y <= 5; y++) {
            RenderObject tri;
            tri.model = engine->_meshManager->get_model("triangle");
            tri.material = engine->_pipelineBuilder->get_material("monkeyMaterial");
            glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y)); // vec3(x, 0, y)
            glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
            tri.transformMatrix = translation * scale;
            renderables.push_back(tri);
        }
    }

    return renderables;
}

Renderables SceneListing::damagedHelmet(Camera& camera, VulkanEngine* engine) {
    Renderables renderables{};

    camera.inverse(false);
    camera.set_position({ 0.0f, 0.0f, -3.0f }); // Re-initialize position after scene change = camera jumping.
    camera.set_perspective(70.f,  (float)engine->_window->_windowExtent.width /(float)engine->_window->_windowExtent.height, 0.1f, 200.0f);  // 1700.f / 1200.f
    camera.type = Camera::Type::look_at;

    std::shared_ptr<ModelGLB> helmetModel = std::make_shared<ModelGLB>(engine->_device.get());
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