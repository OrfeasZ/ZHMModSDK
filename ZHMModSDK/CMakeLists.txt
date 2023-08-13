cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS
	Src/*.cpp
	Src/*.c
	Src/*.hpp
	Src/*.h,
)

file(GLOB_RECURSE HEADER_FILES
	CONFIGURE_DEPENDS
	Include/*.h
)

add_library(ZHMModSDK SHARED
	${SRC_FILES}
	${HEADER_FILES}
	${CMAKE_SOURCE_DIR}/External/RakNet/Source/RakNetTypes.cpp
	${CMAKE_SOURCE_DIR}/External/RakNet/Source/SuperFastHash.cpp
	${CMAKE_SOURCE_DIR}/External/RakNet/Source/RakNetSocket2.cpp
	${CMAKE_SOURCE_DIR}/External/RakNet/Source/GetTime.cpp
	${CMAKE_SOURCE_DIR}/External/RakNet/Source/Itoa.cpp
	${CMAKE_SOURCE_DIR}/External/RakNet/Source/LocklessTypes.cpp
	${CMAKE_SOURCE_DIR}/External/RakNet/Source/RakThread.cpp
	${CMAKE_SOURCE_DIR}/External/RakNet/Source/RakSleep.cpp
)

target_include_directories(ZHMModSDK PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Src
	${CMAKE_SOURCE_DIR}/External/mINI/src/mini
	${CMAKE_SOURCE_DIR}/External/RakNet/Source/
)

target_include_directories(ZHMModSDK PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/Include
	${CMAKE_SOURCE_DIR}/External/IconFontCppHeaders
)

target_compile_definitions(ZHMModSDK PRIVATE
	LOADER_EXPORTS
)

target_link_options(ZHMModSDK PRIVATE
	/DELAYLOAD:d3d12.dll
	/DELAYLOAD:dxgi.dll
)

target_link_libraries(ZHMModSDK PUBLIC
	minhook::minhook
	spdlog::spdlog_header_only
	Microsoft::DirectXTK12
	Microsoft::DirectX-Guids
	Microsoft::DirectX-Headers
	imgui
	d3d12
	dxgi
	delayimp
	ws2_32
)

install(TARGETS ZHMModSDK
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

# Install ZHMModSDK header files.
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Include/"
	DESTINATION include
	FILES_MATCHING PATTERN "*.h"
)

# Install ZHMModSDK pdb file.
install(FILES $<TARGET_PDB_FILE:ZHMModSDK> DESTINATION bin)

# Install spdlog and fmt header files.
install(DIRECTORY "${VCPKG_BUILD_ROOT}/include/spdlog/"
	DESTINATION include/spdlog
	FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY "${VCPKG_BUILD_ROOT}/include/fmt/"
	DESTINATION include/fmt
	FILES_MATCHING PATTERN "*.h"
)

# Install fmt lib.
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	install(FILES "${VCPKG_BUILD_ROOT}/debug/lib/fmtd.lib" DESTINATION lib RENAME fmt.lib)
else()
	install(FILES "${VCPKG_BUILD_ROOT}/lib/fmt.lib" DESTINATION lib)
endif()

# Install icon header.
install(FILES "${CMAKE_SOURCE_DIR}/External/IconFontCppHeaders/IconsMaterialDesign.h" DESTINATION include)