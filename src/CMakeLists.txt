# Add source to this project's executable.

file(GLOB_RECURSE ENGINE "*.cpp" "*.h")
file(GLOB_RECURSE CORE "core/*.cpp" "core/*.h") #  "assets/*.cpp" "assets/*.h"
file(GLOB_RECURSE COMPONENTS "scenes/*.cpp" "scenes/*.h")
file(GLOB_RECURSE SCENES "scenes/*.cpp" "scenes/*.h")
file(GLOB_RECURSE UI "ui/*.cpp" "ui/*.h")
file(GLOB_RECURSE TECHNIQUES "techniques/*.cpp" "techniques/*.h")
# file(GLOB_RECURSE TRACY_CLIENT "../third_party/tracy/public/TracyClient.cpp")
add_executable(h2vk ${ENGINE} ${CORE} ${COMPONENTS} ${SCENES} ${UI} ${TECHNIQUES} ${TRACY_CLIENT})

set_property(TARGET h2vk PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:h2vk>")

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

## === Get Shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/src/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/src/shaders/*.comp"
)

### Iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_DIR ${GLSL} DIRECTORY)
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${FILE_DIR}/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
    ## execute glslang command to compile that specific shader
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)
add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(h2vk shaders) # keep

target_include_directories(h2vk PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}") # keep
target_link_libraries(h2vk vk-bootstrap VulkanMemoryAllocator glm tinyobjloader tinygltf imgui stb_image fonts glfw) # ${LIBGLFW3} ${LIBVULKAN13} ${LIBVULKAN1} # keep
# target_link_libraries(h2vk Tracy::TracyClient) # keep

#find_library(LIBVULKAN13 libvulkan.1.3.204.dylib ~/VulkanSDK/1.3.204.1/macOS/lib/)
#find_library(LIBVULKAN1 libvulkan.1.dylib ~/VulkanSDK/1.3.204.1/macOS/lib/)
#find_library(LIBGLFW3 libglfw.3.3.dylib /usr/local/Cellar/glfw/3.3.6/lib/)
#find_library(GLM glm /usr/local/Cellar/glm/0.9.9.8/lib/)