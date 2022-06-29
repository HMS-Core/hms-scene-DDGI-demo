cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)

set(NAME DDGIExample)
project(${NAME})

set(SRC_DIR ../../../examples/${NAME})
set(BASE_DIR ../../../base)
set(EXTERNAL_DIR ../../../3rdparty)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DVK_USE_PLATFORM_ANDROID_KHR -DVK_NO_PROTOTYPES")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DVK_USE_PLATFORM_ANDROID_KHR")

file(GLOB EXAMPLE_SRC "${SRC_DIR}/*.cpp")

if (ANDROID)
    include_directories($ENV{ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include)
else ()
    find_package(Vulkan REQUIRED)
    if (${VULKAN_FOUND})
        set(VULKAN_DIR ${Vulkan_INCLUDE_DIR})
        set(vulkan-lib ${Vulkan_LIBRARY})
    else ()
        message(FATAL_ERROR "Vulkan Not Found!")
    endif ()
    include_directories(${VULKAN_DIR})
endif ()

add_library(native-lib SHARED ${EXAMPLE_SRC})

add_library(native-app-glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

add_subdirectory(../base ${CMAKE_SOURCE_DIR}/../base)

# build ddgi.so with source code
# build rtcore.so with source code
# compile rayshop src for debug
option(BUILD_DDGI_WITH_CODE "build DDGI with source code" OFF)
option(BUILD_RTCORE_WITH_CODE "build rtcore with source code" OFF)

if (BUILD_RTCORE_WITH_CODE)
    if(NOT RTCORE_DIR)
        set(RTCORE_DIR ../../../../rtcore)
    endif()
    add_subdirectory(${RTCORE_DIR}/../platform/huawei_securec ${RTCORE_DIR}/../platform/huawei_securec/out)
    add_subdirectory(${RTCORE_DIR} ${RTCORE_DIR}/out)
else ()
    # librtcore.so导入
    ADD_LIBRARY(rtcore SHARED IMPORTED)
    SET_TARGET_PROPERTIES(rtcore
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/src/main/libs/librtcore.so)
endif()

if (BUILD_DDGI_WITH_CODE)
    add_definitions(-DBUILD_DDGI_SAMPLE)
    if(NOT DDGISDK_DIR)
        set(DDGISDK_DIR ../../../../DDGISDK)
    endif()
    add_subdirectory(${DDGISDK_DIR} ${DDGISDK_DIR}/out)
else ()
    # libddgi.so导入
    ADD_LIBRARY(ddgi SHARED IMPORTED)
    SET_TARGET_PROPERTIES(ddgi
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/src/main/libs/libddgi.so)
endif ()

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

include_directories(../../../include)
include_directories(${BASE_DIR})
include_directories(${EXTERNAL_DIR})
include_directories(${EXTERNAL_DIR}/glm)
include_directories(${EXTERNAL_DIR}/imgui)
include_directories(${EXTERNAL_DIR}/tinygltf)
include_directories($ENV{ANDROID_NDK_HOME}/sources/android/native_app_glue)

target_link_libraries(
    native-lib
    native-app-glue
    libbase
    rtcore
    ddgi
    android
    log
    z
)
