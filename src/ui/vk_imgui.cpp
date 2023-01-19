#include "vk_imgui.h"
#include "vk_engine.h"
#include "core/vk_window.h"
#include "core/utilities/vk_helpers.h"
#include "core/utilities/vk_types.h"
#include "scenes/vk_scene_listing.h"
#include "core/vk_command_buffer.h"

#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

UInterface::UInterface(VulkanEngine& engine, Settings settings) : _engine(engine), _settings(settings) {
    this->p_open.emplace(SCENE_EDITOR, false);
    this->p_open.emplace(TEXTURE_VIEWER, false);
    this->p_open.emplace(STATS_VIEWER, false);
    this->p_open.emplace(SKYBOX_EDITOR, true);
};

UInterface::~UInterface() {
    this->clean_up();
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

    // ImGui::ShowDemoWindow()
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Tools")) {
            updated |= ImGui::MenuItem("Scene editor", nullptr, &this->p_open[SCENE_EDITOR]);
            updated |= ImGui::MenuItem("Texture viewer", nullptr, &this->p_open[TEXTURE_VIEWER]);
            updated |= ImGui::MenuItem("Statistics viewer", nullptr, &this->p_open[STATS_VIEWER]);
            updated |= ImGui::MenuItem("Enable skybox", nullptr, &this->p_open[SKYBOX_EDITOR]);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        // ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(viewport, ImGuiDockNodeFlags_NoDockingInCentralNode);

        //ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Once);
        // ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
        // ImGui::SetNextWindowSize(ImVec2(viewport->Size.x / 3, viewport->Size.y / 2));
        updated |= this->scene_editor();

        // ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Once);
        // ImGui::SetNextWindowPos(ImVec2(0, viewport->Size.y / 2 + ImGui::GetFrameHeight()));
        // ImGui::SetNextWindowSize(ImVec2(viewport->Size.x / 3, viewport->Size.y / 2 - ImGui::GetFrameHeight()));
        updated |= this->texture_viewer();

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

    const auto window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse ;
    if (ImGui::Begin("Scene editor", &this->p_open[SCENE_EDITOR], window_flags)) {
        std::vector<const char*> scenes;
        for (const auto& scene : SceneListing::scenes) {
            scenes.push_back(scene.first.c_str());
        }

        ImGui::Text("Scene");
        ImGui::Separator();
        ImGui::PushItemWidth(-1);
        updated |= ImGui::Combo("##SceneList", &get_settings().scene_index, scenes.data(), static_cast<int>(scenes.size()));
        ImGui::PopItemWidth();
        ImGui::NewLine();

        ImGui::Text("Camera");
        ImGui::Separator();
        updated |= ImGui::SliderFloat("Speed", &get_settings().speed, 0.01f, 100.0f);
        ImGui::SliderFloat("FOV", &get_settings().fov, 0.0f, 360.0f);
        // ImGui::SliderFloat("Aspect", &get_settings().aspect, 0.0f, 1.0f);
        updated |= ImGui::SliderFloat("Z-Near", &get_settings().z_near, 0.0f, 10.0f);
        updated |= ImGui::SliderFloat("Z-Far", &get_settings().z_far, 0.0f, 500.0f);
        ImGui::NewLine();

        ImGui::Text("Light");
        ImGui::Separator();
        updated |= ImGui::InputFloat3("Position", get_settings().coordinates, 0);
        updated |= ImGui::SliderFloat("Ambient", &get_settings().ambient, 0.0f, 0.005f, "%.4f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
        updated |= ImGui::SliderFloat("Specular", &get_settings().specular, 0.0f, 1.0f);
        updated |= ImGui::InputInt3("Color (RGB)", get_settings().colors, 0);
    }

    ImGui::End();

    return updated;
}

bool UInterface::texture_viewer() {
    bool updated = false;

    if (!this->p_open[TEXTURE_VIEWER]) {
        return false;
    }

    const auto window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse ;
    if (ImGui::Begin("Texture viewer", &this->p_open[TEXTURE_VIEWER], window_flags)) {
        ImGui::Text("TO DO");
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

    if (ImGui::Begin("Statistics viewer", &this->p_open[STATS_VIEWER], flags))
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

