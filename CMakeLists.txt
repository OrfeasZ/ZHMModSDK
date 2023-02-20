cmake_minimum_required (VERSION 3.8)

if(POLICY CMP0091)
	cmake_policy(SET CMP0091 NEW)
endif()

project(ZHMMods CXX C)

set(INSTALL_PATH "" CACHE PATH "Path to Hitman 3 retail folder to install the SDK in.")

set(CMAKE_CXX_STANDARD 23)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(VCPKG_CRT_LINKAGE "static")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	
	string(REPLACE "/MDd" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
endif()

# Dependencies.
find_package(7zip CONFIG REQUIRED)
find_package(directxtk12 CONFIG REQUIRED)
find_package(minhook CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)
add_subdirectory("External/imgui")
add_subdirectory("External/imguizmo")

# Core modules.
add_subdirectory("ZHMModSDK")
add_subdirectory("DirectInputProxy")

# Mods.
set(MODS
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
	Editor
	Clumsy
)

foreach(MOD IN LISTS MODS)
	add_subdirectory("Mods/${MOD}")
endforeach()

# Tools.
add_subdirectory("Tools/DevLoader")

# Make sure to compile everything before the devloader.
add_dependencies(DevLoader 
	DirectInputProxy 
	ZHMModSDK 
	${MODS}
)

if (EXISTS "${INSTALL_PATH}")
	install(TARGETS ZHMModSDK DirectInputProxy 
		RUNTIME DESTINATION "${INSTALL_PATH}"
	)

	install(TARGETS ${MODS}
		RUNTIME DESTINATION "${INSTALL_PATH}/mods"
	)
endif()