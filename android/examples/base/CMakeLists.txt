file(GLOB BASE_SRC "../../../3rdparty/SaschaWillems/*.cpp" "../../../3rdparty/imgui/*.cpp")

include_directories(${BASE_DIR})
include_directories(../../../include)
include_directories(../../../3rdparty)
include_directories(../../../3rdparty/glm)
include_directories(../../../3rdparty/gli)
include_directories(../../../3rdparty/imgui)
include_directories(${EXTERNAL_DIR}/tinygltf)

include_directories($ENV{ANDROID_NDK_HOME}/sources/android/native_app_glue)

add_library(libbase SHARED ${BASE_SRC})

set(KTX_DIR ../../../3rdparty/ktx)
set(KTX_SOURCES
	${KTX_DIR}/lib/texture.c
	${KTX_DIR}/lib/hashlist.c
	${KTX_DIR}/lib/checkheader.c
	${KTX_DIR}/lib/swap.c
	${KTX_DIR}/lib/memstream.c
	${KTX_DIR}/lib/filestream.c
)
set(KTX_INCLUDE
	${KTX_DIR}/include
	${KTX_DIR}/lib
	${KTX_DIR}/other_include
)

add_library(libktx ${KTX_SOURCES})
target_include_directories(libktx PUBLIC ${KTX_INCLUDE})
set_property(TARGET libktx PROPERTY FOLDER "3rdparty")


target_link_libraries(
	libbase
	android
	log
	z
	libktx
	vulkan
)
