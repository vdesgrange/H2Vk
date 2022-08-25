#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <deque>
#include <functional>
#include <math.h>

#include "vk_mesh.h"
#include "vk_types.h"

const uint32_t WIDTH = 1700;
const uint32_t HEIGHT = 1200;

const bool enableValidationLayers = true;
const uint32_t MAX_FRAMES_IN_FLIGHT = 1;

const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)(); //call the function
        }

        deletors.clear();
    }
};

class VulkanEngine {
public:
	bool _isInitialized{ false };
	int _frameNumber {0};

    GLFWwindow* _window;
	VkExtent2D _windowExtent{ WIDTH , HEIGHT };
    bool framebufferResized = false;

    VkInstance _instance; // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
    VkPhysicalDevice _physicalDevice; // GPU chosen as the default device
    VkDevice _device; // Vulkan device for commands
    VkSurfaceKHR _surface; // Vulkan window surface

    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;

    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;;

    VkRenderPass _renderPass;
    std::vector<VkFramebuffer> _frameBuffers;

    VkSwapchainKHR _swapchain;
    VkFormat _swapChainImageFormat;
    std::vector<VkImage> _swapChainImages;
    std::vector<VkImageView> _swapChainImageViews;

    VkSemaphore _presentSemaphore, _renderSemaphore;
    VkFence _renderFence;

    VkPipelineLayout _pipelineLayout;

    VkPipeline _graphicsPipeline;
    VkPipeline _redTrianglePipeline;
    VkPipeline _meshPipeline;

    VmaAllocator _allocator;
    Mesh _mesh;

    DeletionQueue _mainDeletionQueue;
    DeletionQueue _swapChainDeletionQueue;

    int _selectedShader{ 0 };

    //initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

    void recreate_swap_chain();

	//draw loop
	void draw();

	//run main loop
	void run();;

private:
    // Initialize vulkan
    void init_vulkan();

    void init_window();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    void init_swapchain();

    void init_commands();

    void init_command_pool();

    void init_command_buffer();

    void init_default_renderpass();

    void init_framebuffers();

    void init_sync_structures();

    void init_pipelines();

    std::vector<uint32_t> read_file(const char* filePath);

    bool create_shader_module(const std::vector<uint32_t>& code, VkShaderModule* out);

    bool load_shader_module(const char* filePath, VkShaderModule* out);

    void load_meshes();

    void upload_mesh(Mesh& mesh);

//
//    std::vector<const char*> get_required_extensions();
//
//    bool check_validation_layer_support(const std::vector<const char*>& validationLayers);
//
//    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
//
//    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
//            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//            VkDebugUtilsMessageTypeFlagsEXT messageType,
//            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
//            void *pUserData);
//
//    void setup_debug_messenger();
//
//    void create_instance();

//    void createLogicalDevice(const std::vector<const char*>& deviceExtensions,
//                                           const std::vector<const char*> validationLayers,
//                                           const bool enableValidationLayers);
//
//    void pickPhysicalDevice(const std::vector<const char*>& deviceExtensions);
//
//    bool isDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions);
//
//    bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions);
//
//    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice& device, VkSurfaceKHR& surface);
//
//    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
};
