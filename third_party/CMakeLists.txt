find_package(Vulkan REQUIRED)

add_subdirectory(glm INTERFACE)
add_subdirectory(tinygltf)
add_subdirectory(tinyobjloader)
add_subdirectory(vk-bootstrap)
add_subdirectory(VulkanMemoryAllocator)

add_library(imgui STATIC)
target_sources(imgui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
        )
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan)

# stb_image header only lib so we only need the include path
add_library(stb_image INTERFACE)

#find_library(LIBVULKAN13 libvulkan.1.3.204.dylib ~/VulkanSDK/1.3.204.1/macOS/lib/)
#find_library(LIBVULKAN1 libvulkan.1.dylib ~/VulkanSDK/1.3.204.1/macOS/lib/)

#if (WIN32)
#target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include ${sdl2_image_DIR}/include)
#target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib/x64 ${sdl2_image_DIR}/lib/x64)
#elseif (UNIX)
#target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include/SDL2 ${sdl2_image_DIR}/include/SDL2)
#target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib ${sdl2_image_DIR}/lib)
#endif()
