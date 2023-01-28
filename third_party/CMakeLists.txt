find_package(Vulkan REQUIRED)

add_library(glm INTERFACE)
# add_library(vma INTERFACE)
add_library(stb_image INTERFACE)
add_library(tinyobjloader STATIC)
# add_library(tinygltf STATIC)
add_subdirectory(tinygltf)
add_subdirectory(vk-bootstrap) # keep
add_subdirectory(VulkanMemoryAllocator) # keep

#both vma and glm and header only libs so we only need the include path
# target_include_directories(vma INTERFACE vma)

target_include_directories(glm INTERFACE glm)

target_sources(tinyobjloader PRIVATE
    tinyobjloader/tiny_obj_loader.h
    tinyobjloader/tiny_obj_loader.cc
    )
target_include_directories(tinyobjloader PUBLIC tinyobjloader)

#target_sources(tinygltf PRIVATE tinygltf/tiny_gltf.h tinygltf/tiny_gltf.cc)
#set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
#set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
#target_include_directories(tinygltf PUBLIC tinygltf)

#if (WIN32)
#target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include ${sdl2_image_DIR}/include)
#target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib/x64 ${sdl2_image_DIR}/lib/x64)
#elseif (UNIX)
#target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include/SDL2 ${sdl2_image_DIR}/include/SDL2)
#target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib ${sdl2_image_DIR}/lib)
#endif()

# target_link_libraries(sdl2 INTERFACE ${SDL2_LIBRARY}) # SDL2 SDL2main

add_library(imgui STATIC)

target_include_directories(imgui PUBLIC imgui)

target_sources(imgui PRIVATE 
    imgui/imgui.h
    imgui/imgui.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp

    imgui/backends/imgui_impl_vulkan.cpp
    imgui/backends/imgui_impl_glfw.cpp
    )

target_link_libraries(imgui PUBLIC Vulkan::Vulkan)

target_include_directories(stb_image INTERFACE stb_image)

#find_library(LIBVULKAN13 libvulkan.1.3.204.dylib ~/VulkanSDK/1.3.204.1/macOS/lib/)
#find_library(LIBVULKAN1 libvulkan.1.dylib ~/VulkanSDK/1.3.204.1/macOS/lib/)
