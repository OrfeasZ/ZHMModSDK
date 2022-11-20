cmake_minimum_required (VERSION 3.8)

if(POLICY CMP0091)
	cmake_policy(SET CMP0091 NEW)
endif()

project(ZHMMods CXX)

set(CTT OFF CACHE BOOL "Build with support for the Hitman 3 CTT.")

set(CMAKE_CXX_STANDARD 23)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(VCPKG_CRT_LINKAGE "static")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	
	string(REPLACE "/MDd" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
endif()

# Dependencies.
add_subdirectory("External/spdlog")
add_subdirectory("External/minhook")
add_subdirectory("External/imgui")
add_subdirectory("External/DirectXTK12")

# Core modules.
add_subdirectory("ZHMModSDK")
add_subdirectory("DirectInputProxy")

# Mods.
add_subdirectory("Mods/WakingUpNpcs")
add_subdirectory("Mods/NoPause")
add_subdirectory("Mods/SkipIntro")
add_subdirectory("Mods/CodeGen")
add_subdirectory("Mods/CertPinBypass")
add_subdirectory("Mods/MaxPatchLevel")
add_subdirectory("Mods/DebugMod")
add_subdirectory("Mods/AdvancedRating")
add_subdirectory("Mods/FreeCam")
add_subdirectory("Mods/DiscordRichPresence")

# Tools.
add_subdirectory("Tools/DevLoader")

# Make sure to compile everything before the devloader.
add_dependencies(DevLoader 
	DirectInputProxy 
	ZHMModSDK 
	WakingUpNpcs
	NoPause
	SkipIntro
	CodeGen
	CertPinBypass
	MaxPatchLevel
	DebugMod
	AdvancedRating
	FreeCam
	DiscordRichPresence
)