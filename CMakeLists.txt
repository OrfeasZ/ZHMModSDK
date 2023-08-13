cmake_minimum_required(VERSION 3.15)
include(FetchContent)

project(ZHMMods CXX)

set(GAME_INSTALL_PATH "" CACHE PATH "Path to Hitman 3 installation folder.")
set(CMAKE_CXX_STANDARD 23)
set(VCPKG_BUILD_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

# Add CPM.
include(cmake/get-cpm.cmake)

# Dependencies.
find_package(7zip CONFIG REQUIRED)
find_package(directxtk12 CONFIG REQUIRED)
find_package(minhook CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)
find_package(libuv CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(simdjson CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
CPMAddPackage("gh:ValveSoftware/GameNetworkingSockets@1.4.1")
CPMAddPackage("gh:felixguendling/cista@0.14")
CPMAddPackage("gh:OrfeasZ/ZHMTools@3.3.1")
#add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../ZHMTools" "${VCPKG_BUILD_ROOT}/zhmtools")
add_subdirectory(External/imgui)
add_subdirectory(External/imguizmo)

# Core modules.
add_subdirectory(ZHMModSDK)
add_subdirectory(DirectInputProxy)

# Mods.
set(MODS
	WakingUpNpcs
	NoPause
	SkipIntro
	CertPinBypass
	MaxPatchLevel
	DebugMod
	AdvancedRating
	FreeCam
	DiscordRichPresence
	Editor
	Clumsy
	Noclip
	Hitmen
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

# Install ZHMModSDK, dinput8, and all the mods in the game folder.
if (EXISTS "${GAME_INSTALL_PATH}/Retail")
	install(FILES "$<TARGET_FILE:DirectInputProxy>" DESTINATION "${GAME_INSTALL_PATH}/Retail")
	install(FILES "$<TARGET_FILE:ZHMModSDK>" DESTINATION "${GAME_INSTALL_PATH}/Retail")

	foreach(MOD IN LISTS MODS)
		install(FILES "$<TARGET_FILE:${MOD}>" DESTINATION "${GAME_INSTALL_PATH}/Retail/mods")
    endforeach()

    # Install ResourceLib, needed by the Editor mod.
    install(FILES "$<TARGET_FILE:ResourceLib_HM3>" DESTINATION "${GAME_INSTALL_PATH}/Retail")
endif()

# Collect all licenses from ${VCPKG_BUILD_ROOT}/share/<lib>/copyright into licenses/<lib>.txt
install(CODE "set(VCPKG_BUILD_ROOT \"${VCPKG_BUILD_ROOT}\")")
install(CODE "set(INSTALL_DIR \"${CMAKE_INSTALL_PREFIX}\")")
install(CODE [[
	message(STATUS "Collecting licenses from ${VCPKG_BUILD_ROOT}/share/")
	file(GLOB_RECURSE LICENSES "${VCPKG_BUILD_ROOT}/share/*/copyright")

	foreach(LICENSE IN LISTS LICENSES)
		# Get the name of the library (containing folder).
		get_filename_component(LICENSE_DIR "${LICENSE}" DIRECTORY)
		get_filename_component(LICENSE_DIR "${LICENSE_DIR}" NAME)

		message(STATUS "Installing ${LICENSE} as licenses/${LICENSE_DIR}.txt")
		file(COPY "${LICENSE}" DESTINATION "${INSTALL_DIR}/licenses")
		file(RENAME "${INSTALL_DIR}/licenses/copyright" "${INSTALL_DIR}/licenses/${LICENSE_DIR}.txt")
	endforeach()
]])

# Install licenses from external projects.
install(FILES "External/imgui/source/LICENSE.txt" DESTINATION licenses RENAME imgui.txt)
install(FILES "External/imguizmo/source/LICENSE" DESTINATION licenses RENAME imguizmo.txt)
install(FILES "External/mINI/LICENSE" DESTINATION licenses RENAME mINI.txt)
install(FILES "External/IconFontCppHeaders/licence.txt" DESTINATION licenses RENAME IconFontCppHeaders.txt)

# Install licenses from CPM packages.
foreach(PACKAGE ${CPM_PACKAGES})
	message(STATUS "Checking for licenses in ${PACKAGE} (${${PACKAGE}_SOURCE_DIR})...")
	file(GLOB CPM_PACKAGE_LICENSES
		"${${PACKAGE}_SOURCE_DIR}/license*"
		"${${PACKAGE}_SOURCE_DIR}/licence*"
		"${${PACKAGE}_SOURCE_DIR}/copying*"
		"${${PACKAGE}_SOURCE_DIR}/copyright*"
	)

	# If we found any license files, copy the first one.
	if(CPM_PACKAGE_LICENSES)
		list(GET CPM_PACKAGE_LICENSES 0 CPM_PACKAGE_LICENSE)
		install(FILES "${CPM_PACKAGE_LICENSE}" DESTINATION licenses RENAME "${PACKAGE}.txt")
	endif()
endforeach()