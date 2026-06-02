# Cross-compilation override for directx-dxc.
#
# Installs the upstream Windows DXC binaries (dxc.exe + dxcompiler.dll +
# dxil.dll) under tools/directx-dxc/. When cross-building from Linux we
# invoke dxc.exe via wine (see the directxtk12 overlay), so we want the
# real Windows tool, not the native Linux build.

set(VCPKG_POLICY_DLLS_IN_STATIC_LIBRARY enabled)
set(VCPKG_POLICY_ALLOW_RESTRICTED_HEADERS enabled)
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This directx-dxc overlay is for Linux cross-builds only.")
endif()

set(DIRECTX_DXC_TAG v1.8.2502)
set(DIRECTX_DXC_VERSION 2025_02_20)

vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/microsoft/DirectXShaderCompiler/releases/download/${DIRECTX_DXC_TAG}/dxc_${DIRECTX_DXC_VERSION}.zip"
    FILENAME "dxc_${DIRECTX_DXC_VERSION}.zip"
    SHA512 2381852e0d57be65ab919df00d79cda3564cd3695c6415065680738b66de1fc106106baae322973c6a48337b97e603294ee5118f71559298b7097181e73e4b31
)

vcpkg_download_distfile(LICENSE_TXT
    URLS "https://raw.githubusercontent.com/microsoft/DirectXShaderCompiler/${DIRECTX_DXC_TAG}/LICENSE.TXT"
    FILENAME "LICENSE.${DIRECTX_DXC_VERSION}"
    SHA512 9feaa85ca6d42d5a2d6fe773706bbab8241e78390a9d61ea9061c8f0eeb5a3e380ff07c222e02fbf61af7f2b2f6dd31c5fc87247a94dae275dc0a20cdfcc8c9d
)

vcpkg_extract_source_archive(PACKAGE_PATH ARCHIVE ${ARCHIVE} NO_REMOVE_ONE_LEVEL)

set(DXC_ARCH x64)

file(INSTALL
    "${PACKAGE_PATH}/inc/dxcapi.h"
    "${PACKAGE_PATH}/inc/dxcerrors.h"
    "${PACKAGE_PATH}/inc/dxcisense.h"
    "${PACKAGE_PATH}/inc/d3d12shader.h"
    DESTINATION "${CURRENT_PACKAGES_DIR}/include/${PORT}")

file(INSTALL "${PACKAGE_PATH}/lib/${DXC_ARCH}/dxcompiler.lib"
    DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
if(NOT DEFINED VCPKG_BUILD_TYPE)
    file(INSTALL "${PACKAGE_PATH}/lib/${DXC_ARCH}/dxcompiler.lib"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
endif()

file(INSTALL
    "${PACKAGE_PATH}/bin/${DXC_ARCH}/dxcompiler.dll"
    "${PACKAGE_PATH}/bin/${DXC_ARCH}/dxil.dll"
    DESTINATION "${CURRENT_PACKAGES_DIR}/bin")
if(NOT DEFINED VCPKG_BUILD_TYPE)
    file(INSTALL
        "${PACKAGE_PATH}/bin/${DXC_ARCH}/dxcompiler.dll"
        "${PACKAGE_PATH}/bin/${DXC_ARCH}/dxil.dll"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin")
endif()

file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/tools/${PORT}")
file(INSTALL
    "${PACKAGE_PATH}/bin/${DXC_ARCH}/dxc.exe"
    DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}"
    FILE_PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)
file(INSTALL
    "${PACKAGE_PATH}/bin/${DXC_ARCH}/dxcompiler.dll"
    "${PACKAGE_PATH}/bin/${DXC_ARCH}/dxil.dll"
    DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}")

# directxtk12 hard-codes DXC.EXE in its find_program; provide a same-name
# symlink alongside the lowercase tool the wine wrapper invokes.
file(CREATE_LINK
    "${CURRENT_PACKAGES_DIR}/tools/${PORT}/dxc.exe"
    "${CURRENT_PACKAGES_DIR}/tools/${PORT}/DXC.EXE"
    SYMBOLIC)

configure_file("${CMAKE_CURRENT_LIST_DIR}/directx-dxc-config.cmake.in"
    "${CURRENT_PACKAGES_DIR}/share/${PORT}/directx-dxc-config.cmake" @ONLY)

vcpkg_install_copyright(FILE_LIST "${LICENSE_TXT}")
