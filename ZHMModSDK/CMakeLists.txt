cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS
	Src/*.cpp
	Src/*.c
	Src/*.hpp
	Src/*.h,
	Include/*.h
)

add_library(ZHMModSDK SHARED
	${SRC_FILES}
)

target_include_directories(ZHMModSDK PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Src
	${CMAKE_SOURCE_DIR}/External/mINI/src/mini
)

target_include_directories(ZHMModSDK PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_compile_definitions(ZHMModSDK PRIVATE
	LOADER_EXPORTS
)

target_link_libraries(ZHMModSDK PUBLIC
	minhook::minhook
	spdlog::spdlog_header_only
	Microsoft::DirectXTK12
	imgui
	d3d12.lib
	dxgi.lib
	dxguid.lib
	runtimeobject.lib
)
