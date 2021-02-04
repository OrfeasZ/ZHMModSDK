cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS
	Src/*.cpp
	Src/*.c
	Src/*.hpp
	Src/*.h
	Include/*.hpp
	Include/*.h
)

add_library(ZHMGlacierSDK STATIC
	${SRC_FILES}
)

set_target_properties(ZHMGlacierSDK PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	CXX_STANDARD 20
)

target_include_directories(ZHMGlacierSDK PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_include_directories(ZHMGlacierSDK PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/Include
)