cmake_minimum_required(VERSION 3.14.1 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(NAME ddgi-samples)

project(${NAME})

#include_directories(base)
include_directories(include)

# Use FindVulkan module added with CMAKE 3.7
if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	message(STATUS "Using module to find Vulkan")
	find_package(Vulkan)
endif()

if (ANDROID)
    include_directories($ENV{ANDROID_NDK_HOME}/sources/android/native_app_glue)
endif ()

IF(WIN32)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
ENDIF()

if (ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_ANDROID_KHR")
elseif (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
endif()

IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

# Set preprocessor defines
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")
endif()

# Clang specific stuff
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch-enum")
endif()

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("${CMAKE_SOURCE_DIR}")
add_definitions(-DVK_EXAMPLE_DATA_DIR=\"${CMAKE_SOURCE_DIR}/data/\")
add_definitions(-DBUILD_DDGI_SAMPLE)

# Compiler specific stuff
IF(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
ELSEIF(APPLE)
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc -ObjC++")
	ELSE()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc -xobjective-c++")
	ENDIF()
ENDIF(MSVC)

IF(WIN32)
	link_libraries(${Vulkan_LIBRARY})
ELSEIF(APPLE)
	link_libraries(${Vulkan_LIBRARY} "-framework AppKit" "-framework QuartzCore")
ELSE(WIN32)
	link_libraries(${XCB_LIBRARIES} ${Vulkan_LIBRARY} ${Vulkan_LIBRARY} ${DIRECTFB_LIBRARIES} ${WAYLAND_CLIENT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
ENDIF(WIN32)

#
option(BUILD_DDGI_WITH_CODE "build DDGI with source code" OFF)
option(BUILD_RTCORE_WITH_CODE "build rtcore with source code" OFF)

if (BUILD_RTCORE_WITH_CODE)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../rtcore ${CMAKE_CURRENT_SOURCE_DIR}/../rtcore/out)
else ()
    # rtcore.libµ¼Èë
    add_library(rtcore STATIC IMPORTED)
	set_target_properties(rtcore
		PROPERTIES IMPORTED_LOCATION
		${CMAKE_CURRENT_SOURCE_DIR}/libs/rtcore.lib)
	list(APPEND LINK_LIBS ${rtcore})
endif()

if (BUILD_DDGI_WITH_CODE)
    add_definitions(-DBUILD_DDGI_SAMPLE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../DDGISDK ${CMAKE_CURRENT_SOURCE_DIR}/../DDGISDK/out)
else ()
    # ddgi.libµ¼Èë
    add_library(ddgi STATIC IMPORTED)
	set_target_properties(ddgi
		PROPERTIES IMPORTED_LOCATION
		${CMAKE_CURRENT_SOURCE_DIR}/libs/ddgi.lib)
	list(APPEND LINK_LIBS ${ddgi})
endif()

add_subdirectory(3rdparty/SaschaWillems)
add_subdirectory(examples)

