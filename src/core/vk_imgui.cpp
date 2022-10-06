#include "vk_imgui.h"
#include "vk_engine.h"
#include "vk_window.h"
#include "vk_device.h"
#include "vk_renderpass.h"
#include "vk_helpers.h"
#include "vk_types.h"
#include "vk_scene_listing.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

UInterface::UInterface(VulkanEngine& engine, Settings settings) : _engine(engine), _settings(settings) {};

UInterface::~UInterface() {
    this->clean_up();
}

void UInterface::init_imgui() {
    VkDescriptorPoolSize pool_sizes[] =
            {
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

    VK_CHECK(vkCreateDescriptorPool(_engine._device->_logicalDevice, &pool_info, nullptr, &_imguiPool));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForVulkan(_engine._window->_window, true);

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = _engine._device->_instance;
    init_info.PhysicalDevice = _engine._device->_physicalDevice;
    init_info.Device = _engine._device->_logicalDevice;
    init_info.Queue = _engine._device->get_graphics_queue();
    init_info.DescriptorPool = _imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info, _engine._renderPass->_renderPass);

    _engine._meshManager->immediate_submit([&](VkCommandBuffer cmd) {
        ImGui_ImplVulkan_CreateFontsTexture(cmd);
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void UInterface::new_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UInterface::render(VkCommandBuffer cmd, Statistics statistics) {
    this->new_frame();

    this->interface();
    this->interface_statistics(statistics);

    // It might be interesting to use a dedicated commandBuffer (not mainCommandBuffer).
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);
}

void UInterface::clean_up() {
    vkDestroyDescriptorPool(_engine._device->_logicalDevice, _imguiPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UInterface::demo() {
    ImGui::ShowDemoWindow();
    // ImGui::EndFrame(); // Call by ImGui::Render()
}

void UInterface::interface() {
    static bool p_open = false;

    const auto window_flags = ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Settings", &get_settings().p_open, window_flags))
    {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }

    std::vector<const char*> scenes;
    for (const auto& scene : SceneListing::scenes) {
        scenes.push_back(scene.first.c_str());
    }

    ImGui::Text("Help");
    ImGui::Separator();
    ImGui::Checkbox("Enable Statistics.", &get_settings().p_overlay);

    ImGui::Text("Scene");
    ImGui::Separator();
    ImGui::PushItemWidth(-1);
    ImGui::Combo("##SceneList", &get_settings().scene_index, scenes.data(), static_cast<int>(scenes.size()));
    ImGui::PopItemWidth();
    ImGui::NewLine();

    ImGui::Text("Camera");
    ImGui::Separator();
    ImGui::SliderFloat("Speed", &get_settings().speed, 0.01f, 20.0f);
    ImGui::InputInt3("X/Y/Z", get_settings().coordinates, 0);
    ImGui::SliderFloat("FOV", &get_settings().fov, 0.0f, 360.0f);
    ImGui::SliderFloat("Aspect", &get_settings().aspect, 0.0f, 1.0f);
    ImGui::SliderFloat("Z-Near", &get_settings().z_near, 0.0f, 10.0f);
    ImGui::SliderFloat("Z-Far", &get_settings().z_far, 0.0f, 500.0f);
    ImGui::NewLine();

    ImGui::End();

    // ImGui::EndFrame(); // Call by ImGui::Render()
}

void UInterface::interface_statistics(const Statistics& statistics) {
    if (!get_settings().p_overlay) {
        return;
    }

    const auto& io = ImGui::GetIO();
    const float distance = 10.0f;
    const ImVec2 pos = ImVec2(io.DisplaySize.x - distance, distance);
    const ImVec2 posPivot = ImVec2(1.0f, 0.0f);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);
    ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("Statistics", &get_settings().p_overlay, flags))
    {
        ImGui::Text("Statistics (%dx%d):", statistics.FramebufferSize.width, statistics.FramebufferSize.height);
        ImGui::Separator();
        ImGui::Text("Frame rate: %.1f fps", statistics.FrameRate);
    }
    ImGui::End();
}

bool UInterface::want_capture_mouse() const
{
    return ImGui::GetIO().WantCaptureMouse;
}
