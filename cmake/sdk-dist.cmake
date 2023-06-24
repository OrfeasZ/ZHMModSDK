cmake_minimum_required(VERSION 3.8)

add_library(ZHMModSDK SHARED IMPORTED GLOBAL)

set(ZHM_BUILD_TYPE "release")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ZHM_BUILD_TYPE "debug")
endif()

# If ZHMMODSDK_DIST_DIR is not defined, load the sdk from the current source directory.
if(NOT DEFINED ZHMMODSDK_DIST_DIR)
    set(ZHMMODSDK_DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${ZHM_BUILD_TYPE}")
endif()

set_target_properties(ZHMModSDK PROPERTIES
    IMPORTED_LOCATION "${ZHMMODSDK_DIST_DIR}/bin/ZHMModSDK.dll"
    IMPORTED_IMPLIB "${ZHMMODSDK_DIST_DIR}/lib/ZHMModSDK.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${ZHMMODSDK_DIST_DIR}/include"
    INTERFACE_LINK_DIRECTORIES "${ZHMMODSDK_DIST_DIR}/lib"
    INTERFACE_COMPILE_DEFINITIONS "SPDLOG_FMT_EXTERNAL"
)

target_link_libraries(ZHMModSDK INTERFACE
    fmt
    imgui
)