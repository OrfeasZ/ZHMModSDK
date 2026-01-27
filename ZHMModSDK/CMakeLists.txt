cmake_minimum_required(VERSION 3.12)

# The current version of the SDK.
set(ZHMMODSDK_VER_MAJOR 4)
set(ZHMMODSDK_VER_MINOR 0)
set(ZHMMODSDK_VER_PATCH 0)

# The current version of the SDK ABI.
# Any change to this is a breaking change.
set(ZHMMODSDK_ABI_VER 1)

# Generate version-related files.
set(ZHMMODSDK_RCVERSION "${ZHMMODSDK_VER_MAJOR},${ZHMMODSDK_VER_MINOR},${ZHMMODSDK_VER_PATCH},0")
set(ZHMMODSDK_RCVERSION_STR "${ZHMMODSDK_VER_MAJOR}.${ZHMMODSDK_VER_MINOR}.${ZHMMODSDK_VER_PATCH}.0")
set(ZHMMODSDK_VERSION_STR "${ZHMMODSDK_VER_MAJOR}.${ZHMMODSDK_VER_MINOR}.${ZHMMODSDK_VER_PATCH}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ZHMModSDK.rc.in ${CMAKE_CURRENT_BINARY_DIR}/ZHMModSDK.rc)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ModSDKVersion.h.in ${CMAKE_CURRENT_SOURCE_DIR}/Include/ModSDKVersion.h)

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
        ${CMAKE_CURRENT_BINARY_DIR}/ZHMModSDK.rc
)

# Set up Rust helper library.
include(FetchContent)
FetchContent_Declare(
        Corrosion
        GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
        GIT_TAG v0.5.2
)

set(Rust_TOOLCHAIN nightly)
set(Rust_CARGO_TARGET x86_64-pc-windows-msvc)
FetchContent_MakeAvailable(Corrosion)

# Import the Rust helper library.
corrosion_import_crate(MANIFEST_PATH Rust/Cargo.toml)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    corrosion_set_env_vars(zhmmodsdk_rs "CFLAGS=-MTd" "CXXFLAGS=-MTd")
else ()
    corrosion_set_env_vars(zhmmodsdk_rs "CFLAGS=-MT" "CXXFLAGS=-MT")
endif ()

# Create a target to generate C headers for the Rust helper library.
add_custom_target(ZHMModSDK_RustHeaders
        COMMAND cargo test --features c-headers -- generate_headers
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Rust
        COMMENT "Generating C headers for the Rust helper library..."
        VERBATIM
)
add_dependencies(ZHMModSDK ZHMModSDK_RustHeaders)

target_include_directories(ZHMModSDK PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
        ${CMAKE_CURRENT_SOURCE_DIR}/Rust
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
        semver::semver
        simdjson::simdjson
        imgui::imgui
        d3d12
        dxgi
        delayimp
        ws2_32
        winhttp
        comctl32
        sentry::sentry
        zhmmodsdk_rs
        ResourceLib_HM3
        implot
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

# Install the crashpad handler helper.
install(FILES "${VCPKG_BUILD_ROOT}/tools/sentry-native/crashpad_handler.exe" DESTINATION bin)

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
else ()
    install(FILES "${VCPKG_BUILD_ROOT}/lib/fmt.lib" DESTINATION lib)
endif ()

# Install icon header.
install(FILES "${CMAKE_SOURCE_DIR}/External/IconFontCppHeaders/IconsMaterialDesign.h" DESTINATION include)

# Install imgui lib and headers.
install(FILES "${VCPKG_BUILD_ROOT}/lib/imgui.lib" DESTINATION lib)
install(FILES "${VCPKG_BUILD_ROOT}/include/imgui.h" DESTINATION include)
install(FILES "${VCPKG_BUILD_ROOT}/include/imconfig.h" DESTINATION include)
install(FILES "${VCPKG_BUILD_ROOT}/include/imgui_stacklayout.h" DESTINATION include)
install(FILES "${VCPKG_BUILD_ROOT}/include/imgui_stdlib.h" DESTINATION include)

# Install implot lib and headers.
install(FILES "${VCPKG_BUILD_ROOT}/lib/implot.lib" DESTINATION lib)
install(FILES "${VCPKG_BUILD_ROOT}/include/implot.h" DESTINATION include)

# Install ResourceLib.
install(TARGETS ResourceLib_HM3
        RUNTIME DESTINATION bin
)

install(FILES $<TARGET_PDB_FILE:ResourceLib_HM3> DESTINATION bin)