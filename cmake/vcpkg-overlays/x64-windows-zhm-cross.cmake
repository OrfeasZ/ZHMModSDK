set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME "")

# Force every port to use CMP0091 NEW so the static-CRT selection
# (CMAKE_MSVC_RUNTIME_LIBRARY in the chainload toolchain) is actually honored.
list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_POLICY_VERSION_MINIMUM=3.15")

if(${PORT} MATCHES "directx-dxc|xaudio2redist")
        set(VCPKG_CRT_LINKAGE dynamic)
        set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()

get_filename_component(_zhm_toolchain "${CMAKE_CURRENT_LIST_DIR}/../toolchains/clang-cl.cmake" ABSOLUTE)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${_zhm_toolchain}")
