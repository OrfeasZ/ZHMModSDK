cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS
	Src/*.cpp
	Src/*.c
	Src/*.hpp
	Src/*.h
)

add_library(Editor SHARED
	${SRC_FILES}
)

find_path(UWEBSOCKETS_INCLUDE_DIRS "uwebsockets/App.h")
find_path(USOCKETS_LIB "uSockets.lib")

target_include_directories(Editor PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Src
	${UWEBSOCKETS_INCLUDE_DIRS}
)

set(USOCKETS_SEARCH_PATH "${VCPKG_BUILD_ROOT}/lib")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(USOCKETS_SEARCH_PATH "${VCPKG_BUILD_ROOT}/debug/lib")
endif ()

find_library(USOCKETS_LIB
	NAMES uSockets
	PATHS "${USOCKETS_SEARCH_PATH}"
	NO_DEFAULT_PATH
	REQUIRED
)

target_link_libraries(Editor PRIVATE
	ZHMModSDK
	imguizmo
	ws2_32
	cista
	ResourceLib_HM3
	uv_a
	ZLIB::ZLIB
	"${USOCKETS_LIB}"
	simdjson::simdjson
	#	TracyClient
)

install(TARGETS Editor
	RUNTIME DESTINATION bin/mods
)

install(TARGETS ResourceLib_HM3
	RUNTIME DESTINATION bin
)