cmake_minimum_required (VERSION 3.8)
project ("vulkan_guide")

IF(APPLE)
  set(CMAKE_THREAD_LIBS_INIT "-lpthread")
  set(CMAKE_HAVE_THREADS_LIBRARY 1)
  set(CMAKE_USE_WIN32_THREADS_INIT 0)
  set(CMAKE_USE_PTHREADS_INIT 1)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
ENDIF()

set(CMAKE_CXX_STANDARD 17) # keep
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin") # keep
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -O0 ") # keep
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -O0") # keep # -fsanitize=address -fsanitize=thread -fsanitize=undefined
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address -O0") # keep

option(TRACY_ENABLE "" ON) # keep
option(TRACY_ON_DEMAND "" ON) # keep
option(VMA_RECORDING_ENABLED "" ON) # keep
option(VMA_DEBUG_LOG "" ON) # keep

#find_package(Vulkan REQUIRED)

add_subdirectory(src) # keep
add_subdirectory(third_party) # keep
add_subdirectory(third_party/tracy/) # keep

# include_directories(/usr/local/include/ src/)

#set(VK_ICD_FILENAMES ~/VulkanSDK/1.3.204.1/macOS/share/vulkan/icd.d/MoltenVK_icd.json)
#set(VK_LAYER_PATH ~/VulkanSDK/1.3.204.1/macOS/share/vulkan/explicit_layer.d)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

## find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/src/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/src/shaders/*.comp"
    )

## iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_DIR ${GLSL} DIRECTORY)
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${FILE_DIR}/${FILE_NAME}.spv") # ${PROJECT_SOURCE_DIR}/src/shaders/
  message(STATUS ${GLSL})
  ##execute glslang command to compile that specific shader
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )
