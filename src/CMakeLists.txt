# Add source to this project's executable.
file(GLOB_RECURSE CORE "*.cpp" "*.h" "core/*.cpp" "core/*.h" "assets/*.cpp" "assets/*.h") # keep
file(GLOB_RECURSE TRACY_CLIENT "../third_party/tracy/public/TracyClient.cpp") # keep
add_executable(h2vk ${CORE} ${TRACY_CLIENT}) # keep

set_property(TARGET h2vk PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:h2vk>")

target_include_directories(h2vk PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}") # keep
target_link_libraries(h2vk vk-bootstrap vma glm tinyobjloader tinygltf imgui stb_image ${LIBGLFW3}) #  ${LIBVULKAN13} ${LIBVULKAN1} # keep
target_link_libraries(h2vk Tracy::TracyClient) # keep

add_dependencies(h2vk Shaders) # keep

#find_library(LIBVULKAN13 libvulkan.1.3.204.dylib ~/VulkanSDK/1.3.204.1/macOS/lib/)
#find_library(LIBVULKAN1 libvulkan.1.dylib ~/VulkanSDK/1.3.204.1/macOS/lib/)
#find_library(LIBGLFW3 libglfw.3.3.dylib /usr/local/Cellar/glfw/3.3.6/lib/)
#find_library(GLM glm /usr/local/Cellar/glm/0.9.9.8/lib/)