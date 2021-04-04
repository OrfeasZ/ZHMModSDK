cmake_minimum_required (VERSION 3.8)

project(ZHMMods CXX)

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
)