/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_imgui.h"
#include "vk_imgui_wrapper.h"
#include "vk_imgui_controller.h"
#include "vk_engine.h"
#include "components/camera/vk_camera.h"
#include "components/lighting/vk_light.h"
#include "core/utilities/vk_helpers.h"
#include "scenes/vk_scene_listing.h"
#include "core/vk_command_buffer.h"

#include "imgui_internal.h"
#include "icons_font.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

UInterface::UInterface(VulkanEngine& engine, Settings settings) : _engine(engine), _settings(settings) {
    this->p_open.emplace(SCENE_EDITOR, false);
    this->p_open.emplace(STATS_VIEWER, false);
    this->p_open.emplace(VIEW_EDITOR, false);
    this->p_open.emplace(LIGHT_EDITOR, false);
    this->p_open.emplace(LOG_CONSOLE, false);
    this->p_open.emplace(SKYBOX_EDITOR, true);

    _layoutCache = new DescriptorLayoutCache(*engine._device);
    _allocator = new DescriptorAllocator(*engine._device);
};

UInterface::~UInterface() {
    this->clean_up();
}

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void UInterface::init_imgui() {
    VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VK_CHECK(vkCreateDescriptorPool(_engine._device->_logicalDevice, &pool_info, nullptr, &_pool));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.Fonts->AddFontDefault();

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    io.Fonts->AddFontFromFileTTF("../third_party/fonts/fa-regular-400.ttf", 13.0f, &config, icon_ranges);
    io.Fonts->AddFontFromFileTTF("../third_party/fonts/fa-solid-900.ttf", 13.0f, &config, icon_ranges);

    ImGui_ImplGlfw_InitForVulkan(_engine._window->_window, true);

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = _engine._device->_instance;
    init_info.PhysicalDevice = _engine._device->_physicalDevice;
    init_info.Device = _engine._device->_logicalDevice;
    init_info.Queue = _engine._device->get_graphics_queue();
    init_info.DescriptorPool = _pool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info, _engine._renderPass->_renderPass);

    CommandBuffer::immediate_submit(*_engine._device, _engine._uploadContext,[&](VkCommandBuffer cmd) {
        ImGui_ImplVulkan_CreateFontsTexture(cmd);
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

bool UInterface::render(VkCommandBuffer cmd, Statistics statistics) {
    this->new_frame();
    bool updated = this->interface(statistics);

    ImGui::Render();     // It might be interesting to use a dedicated commandBuffer (not mainCommandBuffer).
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);

    return updated;
}

bool UInterface::want_capture_mouse()
{
    return ImGui::GetIO().WantCaptureMouse;
}

void UInterface::clean_up() {
    vkDestroyDescriptorPool(_engine._device->_logicalDevice, _pool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UInterface::new_frame() {
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

bool UInterface::interface(Statistics statistics) {
    const auto& io = ImGui::GetIO();
    bool updated = false;

    // ImGui::ShowDemoWindow();
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Tools")) {
            updated |= ImGui::MenuItem("View tools", nullptr, &this->p_open[VIEW_EDITOR]);
            updated |= ImGui::MenuItem("Scene editor", nullptr, &this->p_open[SCENE_EDITOR]);
            updated |= ImGui::MenuItem("Performances", nullptr, &this->p_open[STATS_VIEWER]);
            updated |= ImGui::MenuItem("Enable skybox", nullptr, &this->p_open[SKYBOX_EDITOR]);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
//        const ImGuiViewport* viewport = ImGui::GetMainViewport();
//        ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(viewport, ImGuiDockNodeFlags_NoDockingInCentralNode);

//        ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Once);
//        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
        // ImGui::SetNextWindowSize(ImVec2(viewport->Size.x / 3, viewport->Size.y / 2));
        updated |= this->view_editor();

//        ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Once);
        updated |= this->scene_editor();

        updated |= this->stats_viewer(statistics);

        updated |= this->skybox_editor();
    }

    return updated;
}

bool UInterface::scene_editor() {
    bool updated = false;

    if (!this->p_open[SCENE_EDITOR]) {
        return false;
    }

    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);

    const auto window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse ;
    if (ImGui::Begin("Inspector", &this->p_open[SCENE_EDITOR], window_flags)) {

        std::vector<const char*> scenes;
        scenes.reserve(SceneListing::scenes.size());
        for (const auto& scene : SceneListing::scenes) {
            scenes.push_back(scene.first.c_str());
        }

        ImGui::Separator();
        updated |= ImGui::Combo("Instances", &get_settings().scene_index, scenes.data(), static_cast<int>(scenes.size()));
        ImGui::NewLine();
        ImGui::Separator();

        ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
            static int selected_object = -1; // todo make a general entity manager
            static std::string selected_light = "";
            static int selected_tex = -1;

            ImGui::PushID("##scene_content");
            ImGui::BeginGroup();

            const ImGuiID child_id = ImGui::GetID((void*)(intptr_t)0);
            float x = ImGui::GetContentRegionAvail().x;
            float y = (selected_object == -1 && selected_tex == -1 && selected_light.empty()) ? ImGui::GetContentRegionAvail().y : ImGui::GetContentRegionAvail().y / 3;
            const bool child_is_visible = ImGui::BeginChild(child_id, ImVec2(x, y), true, 0);

            if (child_is_visible) {
                // Lighting
                for (auto& l : _engine._lightingManager->_entities) {
                    auto light = std::static_pointer_cast<Light>(l.second);
                    std::string label = "   " + std::string(ICON_FA_LIGHTBULB) + " " + Light::types[light->get_type()] + " light";
                    std::string light_id = "##light_" + std::to_string(light->_uid);
                    ImGui::PushID(light_id.c_str());
                    if (ImGui::Selectable(label.c_str(), selected_light == l.first)) {
                        selected_light = l.first;
                        selected_object = -1;
                        selected_tex = -1;
                    }

                    ImGui::PopID();
                }

                // Mesh
                for (uint32_t i = 0; i < _engine._scene->_renderables.size(); i++) {
                    const auto &object = _engine._scene->_renderables[i];

                    ImGuiTreeNodeFlags node_flags = base_flags;
                    node_flags = (selected_object == i) ? node_flags | ImGuiTreeNodeFlags_Selected : node_flags;

                    bool node_open = ImGui::TreeNodeEx((void *) (intptr_t) i, node_flags, "%s %s", ICON_FA_CUBES, object.model->_name.c_str());
                    if (ImGui::IsItemClicked()) { //  && !ImGui::IsItemToggledOpen()
                        selected_object = i;
                        selected_light = "";
                    }

                    if (node_open) {
                        for (const auto& node: object.model->_nodes) {
                            ImGui::Text("%s %s", ICON_FA_CUBE, node->name.c_str());
                        }

                        for (int j = 0; j < object.model->_images.size(); j++) {
                            const auto &image =  object.model->_images[j];
                            std::string label = std::string(ICON_FA_IMAGE) + image._texture._uri;
                            std::string tex_id = "##texture_object_" + std::to_string(i) + std::to_string(j);

                            ImGui::PushID(tex_id.c_str());
                            if (ImGui::Selectable(label.c_str(), selected_tex == j))
                                selected_tex = j;
                            ImGui::PopID();
                        }

                        ImGui::TreePop();
                    }
                }

            }
            ImGui::EndChild();
            ImGui::EndGroup();
            ImGui::PopID();

            ImGui::Separator();
            ImGui::BeginGroup();

            if (ImGui::BeginTabBar("##scene_information", ImGuiTabBarFlags_None)) {

                if (!selected_light.empty()) { //  selected_light < _engine._lightingManager->_entities.size()
                    std::shared_ptr<Light> light = std::static_pointer_cast<Light>(_engine._lightingManager->get_entity(selected_light));

                    if (ImGui::BeginTabItem("Properties")) {
                        ImGui::Text("Name"); ImGui::SameLine(100); ImGui::Text("%s", "Light");
                        ImGui::Text("Global UID"); ImGui::SameLine(100); ImGui::Text("%i", light->_guid);
                        ImGui::Text("Local ID"); ImGui::SameLine(100); ImGui::Text("%i", light->_uid);
                        ImGui::Text("Type"); ImGui::SameLine(100); ImGui::Text("%s", Light::types[light->get_type()]);
                        float color[3] = {light->get_color()[0], light->get_color()[1], light->get_color()[2]};
                        ImGui::Text("Color"); ImGui::SameLine(100); ImGui::ColorEdit3("", color, 0);
                        if (light->get_type() == Light::Type::DIRECTIONAL) {
                            ImGui::Text("Rotation"); ImGui::SameLine(100);
                            updated |= ImGui::InputFloat3("dir", UIController::get_rotation(*light), UIController::set_rotation(*light), "%.2f");
                        } else if (light->get_type() == Light::Type::SPOT) {
                            ImGui::Text("Position"); ImGui::SameLine(100);
                            updated |= ImGui::InputFloat3("pos", UIController::get_position(*light), UIController::set_position(*light), "%.2f");
                            ImGui::Text("Rotation"); ImGui::SameLine(100);
                            updated |= ImGui::InputFloat3("rot", UIController::get_rotation(*light), UIController::set_rotation(*light), "%.2f");
                        }

                        ImGui::EndTabItem();
                    }
                }

                if (selected_object != -1 && selected_object < _engine._scene->_renderables.size()) {

                    const auto &model = _engine._scene->_renderables[selected_object].model;

                    if (ImGui::BeginTabItem("Properties")) {
                        ImGui::Text("Name"); ImGui::SameLine(100); ImGui::Text("%s", model->_name.c_str());
                        ImGui::Text("Unique ID"); ImGui::SameLine(100); ImGui::Text("%i", model->_uid);
                        ImGui::Text("Vertices"); ImGui::SameLine(100); ImGui::Text("%lu", model->_verticesBuffer.size());
                        ImGui::Text("Indexes"); ImGui::SameLine(100); ImGui::Text("%lu", model->_indexesBuffer.size());
                        ImGui::Text("Transform"); ImGui::SameLine(100); ImGui::Text("%i", model->_uid);
                        ImGui::EndTabItem();
                    }

                    if (selected_tex != -1 && selected_tex < model->_images.size()) {
                            auto &image = model->_images[selected_tex]; // .at slower, better checking

                            if (get_settings().texture_index != selected_tex) {
                                get_settings().texture_index = selected_tex;
                                DescriptorBuilder::begin(*_layoutCache, *_allocator)
                                        .bind_image(image._texture._descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                    VK_SHADER_STAGE_FRAGMENT_BIT, 0)
                                        .layout(_engine._descriptorSetLayouts.gui)
                                        .build(get_settings()._textureDescriptorSet, _engine._descriptorSetLayouts.gui,
                                               {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}});
                            }

                            if (ImGui::BeginTabItem("Texture")) {
                                ImGui::Text("URI"); ImGui::SameLine(100); ImGui::Text("%s", image._texture._uri.c_str());

                                float tex_width = fminf(ImGui::GetContentRegionAvail().x, (float)image._texture._width);
                                float tex_height = fminf(ImGui::GetContentRegionAvail().y, (float)image._texture._height);
                                ImGui::Image(get_settings()._textureDescriptorSet, ImVec2(tex_width, tex_height));
                                ImGui::EndTabItem();
                            }
                        }

                }

                ImGui::EndTabBar();
            }
            ImGui::EndGroup();
        }
    }

    ImGui::End();

    return updated;
}

bool UInterface::view_editor() {
    bool updated = false;

    if (!this->p_open[VIEW_EDITOR]) {
        return false;
    }

    ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);

    const auto window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("View Tools", &this->p_open[VIEW_EDITOR], window_flags)) {
        std::vector<const char*> types(Camera::AllType.size());
        for (const auto& type : Camera::AllType) {
            types[type.first] = type.second;
        }

        ImGui::Text("Camera parameters");
        ImGui::Separator();
        bool newMode = ImGui::Combo("Mode", UIController::get_type(*_engine._camera), UIController::set_type(*_engine._camera), types.data(), static_cast<int>(types.size()));
        ImGui::SameLine();
        HelpMarker("'Look at' recommended");

        if (newMode) {
            UIController::set_target(*_engine._camera)({0.0f,  0.0f,  0.0f});
        }
        updated |= newMode;

        if (_engine._camera->get_type() == Camera::Type::look_at) {
                updated |= ImGui::InputFloat3("Target view", UIController::get_target(*_engine._camera), UIController::set_target(*_engine._camera), "%.2f");
            ImGui::SameLine();
            HelpMarker("Fixed at {0, 0, 0} for other modes");
        }
        ImGui::NewLine();

        updated |= ImGui::InputFloat2("Z-Near/Z-Far", UIController::get_z(*_engine._camera), UIController::set_z(*_engine._camera), "%.2f");
        updated |= ImGui::SliderFloat("Angle", UIController::get_fov(*_engine._camera), UIController::set_fov(*_engine._camera), 0.01f, 360.0f);
        updated |= ImGui::SliderFloat("Speed", UIController::get_speed(*_engine._camera), UIController::set_speed(*_engine._camera), 0.01f, 50.0f);
        ImGui::NewLine();

    }
    ImGui::End();

    return updated;
}

bool UInterface::stats_viewer(const Statistics& statistics) {
    bool updated = false;

    if (!this->p_open[STATS_VIEWER]) {
        return false;
    }

    const auto& io = ImGui::GetIO();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    const float distance = 10.0f;
    const ImVec2 pos = ImVec2(io.DisplaySize.x - distance, ImGui::GetFrameHeight());
    const ImVec2 posPivot = ImVec2(1.0f, 0.0f);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x / 3, viewport->Size.y / 5));
    ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("Performances", &this->p_open[STATS_VIEWER], flags))
    {
        ImGui::Text("Statistics (%.0f x %.0f):", io.DisplayFramebufferScale.x * io.DisplaySize.x,  io.DisplayFramebufferScale.y * io.DisplaySize.y);
        ImGui::Separator();
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / statistics.FrameRate, statistics.FrameRate);
//        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
//        ImGui::Text("%d vertices, %d indices ", io.MetricsRenderVertices, io.MetricsRenderIndices);
//        ImGui::Text("%d triangles", io.MetricsRenderIndices / 3);
        ImGui::Text("Coordinates (%.0f, %.0f, %.0f)", statistics.coordinates[0], statistics.coordinates[1], statistics.coordinates[2]);
        ImGui::Text("Rotation (%.0f, %.0f, %.0f)", statistics.rotation[0], statistics.rotation[1], statistics.rotation[2]);
    }
    ImGui::End();

    return updated;
}

bool UInterface::skybox_editor() {
    bool updated = false;

    if (!this->p_open[SKYBOX_EDITOR]) {
        return false;
    }

    return updated;
}

bool UInterface::light_editor() {
    bool updated = false;

    if (!this->p_open[LIGHT_EDITOR]) {
        return false;
    }

    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

    const auto window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse ;
    if (ImGui::Begin("Light Editor", &this->p_open[VIEW_EDITOR], window_flags)) {



        ImGui::Text("Environment");
        ImGui::Separator();

        // todo display skybox texture

    }
    ImGui::End();

//        ImGui::Text("Light");
//        ImGui::Separator();
//        updated |= ImGui::InputFloat3("Position", get_settings().coordinates, "%.2f");
//        updated |= ImGui::SliderFloat("Ambient", &get_settings().ambient, 0.0f, 0.005f, "%.4f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
//        updated |= ImGui::SliderFloat("Specular", &get_settings().specular, 0.0f, 1.0f);
//        updated |= ImGui::InputFloat3("Color (RGB)", get_settings().colors, "%.2f");

    return updated;
}