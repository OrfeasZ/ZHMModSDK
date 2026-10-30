set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

if(${PORT} MATCHES "directx-dxc|xaudio2redist")
	set(VCPKG_CRT_LINKAGE dynamic)
	set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()

# DirectXTex pulls in OpenMP which isn't linked statically, so we turn it off to avoid
# having to ship the VC++ redist.
if(${PORT} STREQUAL "directxtex")
	list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DBC_USE_OPENMP=OFF")
endif()