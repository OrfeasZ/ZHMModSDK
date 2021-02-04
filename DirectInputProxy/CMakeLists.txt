cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS
	Src/*.cpp
	Src/*.c
	Src/*.hpp
	Src/*.h
)

add_library(DirectInputProxy SHARED
	${SRC_FILES}
)

set_target_properties(DirectInputProxy PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	CXX_STANDARD 20
	OUTPUT_NAME "dinput8"
)
