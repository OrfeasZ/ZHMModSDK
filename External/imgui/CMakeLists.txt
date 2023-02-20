cmake_minimum_required(VERSION 3.12)

add_library(imgui STATIC
	source/imgui.cpp
	source/imgui.h
	source/imgui_draw.cpp
	source/imgui_internal.h
	source/imgui_tables.cpp
	source/imgui_widgets.cpp
	source/imstb_rectpack.h
	source/imstb_textedit.h
	source/imstb_truetype.h
	source/imgui_demo.cpp
)

target_include_directories(imgui PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/source
)

add_executable(binary_to_compressed_c
	source/misc/fonts/binary_to_compressed_c.cpp
)