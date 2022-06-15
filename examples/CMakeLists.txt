# Function for building single example
function(buildExample EXAMPLE_NAME)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/glm)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/imgui)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/tinygltf)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/ktx/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/ktx/other_include)
    message("${CMAKE_CURRENT_SOURCE_DIR}")
    if (WIN32)
        add_library(shaderc-lib STATIC IMPORTED)
        set_target_properties(shaderc-lib
            PROPERTIES IMPORTED_LOCATION
            $ENV{VULKAN_SDK}/Lib/shaderc_shared.lib)
        list(APPEND LINK_LIBS ${shaderc-lib})
    endif ()
    SET(EXAMPLE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME})
    message(STATUS "Generating project file for example in ${EXAMPLE_FOLDER}")
    # Main
    file(GLOB SOURCE *.cpp ${BASE_HEADERS} ${EXAMPLE_FOLDER}/*.cpp)
    SET(MAIN_CPP ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.cpp)
    if(EXISTS ${EXAMPLE_FOLDER}/main.cpp)
        SET(MAIN_CPP ${EXAMPLE_FOLDER}/main.cpp)
    ENDIF()

    if(EXISTS ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.h)
        SET(MAIN_HEADER ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.h)
    ENDIF()
    # imgui example requires additional source files
    IF(${EXAMPLE_NAME} STREQUAL "imgui")
        file(GLOB ADD_SOURCE "../external/imgui/*.cpp")
        SET(SOURCE ${SOURCE} ${ADD_SOURCE})
    ENDIF()
    # wayland requires additional source files
    IF(USE_WAYLAND_WSI)
        SET(SOURCE ${SOURCE} ${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h ${CMAKE_BINARY_DIR}/xdg-shell-protocol.c)
    ENDIF()
    # Add shaders
    set(SHADER_DIR_GLSL "../data/shaders/glsl/${EXAMPLE_NAME}")
    file(GLOB SHADERS_GLSL "${SHADER_DIR_GLSL}/*.vert" "${SHADER_DIR_GLSL}/*.frag" "${SHADER_DIR_GLSL}/*.comp" "${SHADER_DIR_GLSL}/*.geom" "${SHADER_DIR_GLSL}/*.tesc" "${SHADER_DIR_GLSL}/*.tese" "${SHADER_DIR_GLSL}/*.mesh" "${SHADER_DIR_GLSL}/*.task" "${SHADER_DIR_GLSL}/*.rgen" "${SHADER_DIR_GLSL}/*.rchit" "${SHADER_DIR_GLSL}/*.rmiss" "${SHADER_DIR_GLSL}/*.rcall")
    set(SHADER_DIR_HLSL "../data/shaders/hlsl/${EXAMPLE_NAME}")
    file(GLOB SHADERS_HLSL "${SHADER_DIR_HLSL}/*.vert" "${SHADER_DIR_HLSL}/*.frag" "${SHADER_DIR_HLSL}/*.comp" "${SHADER_DIR_HLSL}/*.geom" "${SHADER_DIR_HLSL}/*.tesc" "${SHADER_DIR_HLSL}/*.tese" "${SHADER_DIR_HLSL}/*.mesh" "${SHADER_DIR_HLSL}/*.task" "${SHADER_DIR_HLSL}/*.rgen" "${SHADER_DIR_HLSL}/*.rchit" "${SHADER_DIR_HLSL}/*.rmiss" "${SHADER_DIR_HLSL}/*.rcall")
    source_group("Shaders\\GLSL" FILES ${SHADERS_GLSL})
    source_group("Shaders\\HLSL" FILES ${SHADERS_HLSL})
    # Add optional readme / tutorial
    file(GLOB README_FILES "${EXAMPLE_FOLDER}/*.md")
    if(WIN32)
        add_executable(${EXAMPLE_NAME} WIN32 ${MAIN_CPP} ${SOURCE} ${MAIN_HEADER} ${SHADERS_GLSL} ${SHADERS_HLSL} ${README_FILES})
        target_link_libraries(${EXAMPLE_NAME} ${Vulkan_LIBRARY} ${WINLIBS} ddgi shaderc-lib rtcore SaschaWillemsVulkan)
    else(WIN32)
        add_executable(${EXAMPLE_NAME} ${MAIN_CPP} ${SOURCE} ${MAIN_HEADER} ${SHADERS_GLSL} ${SHADERS_HLSL} ${README_FILES})
        target_link_libraries(${EXAMPLE_NAME} ddgi shaderc-lib rtcore SaschaWillemsVulkan )
    endif(WIN32)

    if(RESOURCE_INSTALL_DIR)
        install(TARGETS ${EXAMPLE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endfunction(buildExample)

# Build all examples
function(buildExamples)
    foreach(EXAMPLE ${EXAMPLES})
        buildExample(${EXAMPLE})
    endforeach(EXAMPLE)
endfunction(buildExamples)

set(EXAMPLES
    DDGIExample
)

buildExamples()
