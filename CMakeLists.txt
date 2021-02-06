cmake_minimum_required (VERSION 3.8)

project(ZHMMods CXX)

# Dependencies.
add_subdirectory("External/spdlog")
add_subdirectory("External/minhook")

# Core modules.
add_subdirectory("ZHMGlacierSDK")
add_subdirectory("ZHMModSDK")
add_subdirectory("DirectInputProxy")

# Mods.
add_subdirectory("Mods/WakingUpNpcs")
add_subdirectory("Mods/NoPause")

# Tools.
add_subdirectory("Tools/DevLoader")

# Make sure to compile everything before the devloader.
add_dependencies(DevLoader 
	DirectInputProxy 
	ZHMModSDK 
	WakingUpNpcs
	NoPause
)