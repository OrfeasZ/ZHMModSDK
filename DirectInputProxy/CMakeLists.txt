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
	OUTPUT_NAME "dinput8"
)

install(TARGETS DirectInputProxy
	RUNTIME DESTINATION bin
)