cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS
	Src/*.cpp
	Src/*.c
	Src/*.hpp
	Src/*.h
)

add_library(FreeCam SHARED
	${SRC_FILES}
)

target_include_directories(FreeCam PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_link_libraries(FreeCam PRIVATE
	ZHMModSDK
)

install(TARGETS FreeCam
	RUNTIME DESTINATION bin/mods
)