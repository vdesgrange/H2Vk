cmake_minimum_required (VERSION 3.8)
project ("h2vk")

IF(APPLE)
  set(CMAKE_THREAD_LIBS_INIT "-lpthread")
  set(CMAKE_HAVE_THREADS_LIBRARY 1)
  set(CMAKE_USE_WIN32_THREADS_INIT 0)
  set(CMAKE_USE_PTHREADS_INIT 1)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
ENDIF()

set(CMAKE_CXX_STANDARD 17) # keep
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin") # keep

IF (APPLE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_MACOS_MVK")
  set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG}")
ELSE()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -O0 ") 
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -O0") # -fsanitize=address -fsanitize=thread -fsanitize=undefined
  set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address -O0")
ENDIF()

option(TRACY_ENABLE "" OFF) # keep
option(TRACY_ON_DEMAND "" OFF) # keep
option(VMA_RECORDING_ENABLED "" OFF) # keep
option(VMA_DEBUG_LOG "" OFF) # keep

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_subdirectory(src) # keep
add_subdirectory(third_party) # keep
# add_subdirectory(third_party/tracy/) # keep

# include_directories(/usr/local/include/)

#set(VK_ICD_FILENAMES ~/VulkanSDK/1.3.204.1/macOS/share/vulkan/icd.d/MoltenVK_icd.json)
#set(VK_LAYER_PATH ~/VulkanSDK/1.3.204.1/macOS/share/vulkan/explicit_layer.d)