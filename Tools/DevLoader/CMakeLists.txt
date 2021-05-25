cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS
	Src/*.cpp
	Src/*.c
	Src/*.hpp
	Src/*.h
)

add_executable(DevLoader
	${SRC_FILES}
)

target_include_directories(DevLoader PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_include_directories(DevLoader PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/Include
)
