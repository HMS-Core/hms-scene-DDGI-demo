cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)

file(GLOB BASE_SRC "*.cpp" "*.hpp" "*.h" "../imgui/*.cpp")
file(GLOB BASE_HEADERS "*.hpp" "*.h")

if (ANDROID)
    include_directories($ENV{ANDROID_NDK_HOME}/sources/android/native_app_glue)
endif ()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../glm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../gli)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../imgui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../tinygltf)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ktx/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ktx/other_include)

set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ktx)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c)

add_library(SaschaWillemsVulkan STATIC ${BASE_SRC} ${KTX_SOURCES})

target_link_libraries(SaschaWillemsVulkan ${Vulkan_LIBRARY})
