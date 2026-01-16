cmake_minimum_required(VERSION 3.12)

add_library(implot STATIC
	src/implot.cpp
	src/implot.h
	src/implot_demo.cpp
	src/implot_internal.h
	src/implot_items.cpp
)

target_include_directories(implot PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(implot PUBLIC
	imgui::imgui
)

set_target_properties(implot PROPERTIES
	PUBLIC_HEADER "src/implot.h;"
)

install(TARGETS implot
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)