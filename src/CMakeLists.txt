# Add source to this project's executable.
file(GLOB_RECURSE CORE "*.cpp" "*.h")
add_executable(vulkan_guide ${CORE})

set_property(TARGET vulkan_guide PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkan_guide>")

target_include_directories(vulkan_guide PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

find_library(LIBVULKAN13 libvulkan.1.3.204.dylib ~/VulkanSDK/1.3.204.1/macOS/lib/)
find_library(LIBVULKAN1 libvulkan.1.dylib ~/VulkanSDK/1.3.204.1/macOS/lib/)
find_library(LIBGLFW3 libglfw.3.3.dylib /usr/local/Cellar/glfw/3.3.6/lib/)
find_library(GLM glm /usr/local/Cellar/glm/0.9.9.8/lib/)
target_link_libraries(vulkan_guide vkbootstrap vma glm tinyobjloader imgui stb_image ${LIBVULKAN13} ${LIBVULKAN1} ${LIBGLFW3})
# target_link_libraries(vulkan_guide Vulkan::Vulkan sdl2)

add_dependencies(vulkan_guide Shaders)
