cmake_minimum_required(VERSION 3.12)

add_library(imguizmo STATIC
	source/GraphEditor.cpp
	source/GraphEditor.h
	source/ImCurveEdit.cpp
	source/ImCurveEdit.h
	source/ImGradient.cpp
	source/ImGradient.h
	source/ImGuizmo.cpp
	source/ImGuizmo.h
	source/ImSequencer.cpp
	source/ImSequencer.h
	source/ImZoomSlider.h
)

target_include_directories(imguizmo PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(imguizmo PUBLIC
	imgui
)
