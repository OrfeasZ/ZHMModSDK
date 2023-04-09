cmake_minimum_required(VERSION 3.12)

#### Begin Discord Game SDK
include(FetchContent)

set(DISCORD_SDK_LIB_PATH ${CMAKE_BINARY_DIR}/External/DiscordSdk/lib/x86_64/discord_game_sdk.dll.lib)
set(DISCORD_SDK_SRC_PATH ${CMAKE_BINARY_DIR}/External/DiscordSdk/cpp)

FetchContent_Declare(DiscordSdk
  URL "https://dl-game-sdk.discordapp.net/2.5.6/discord_game_sdk.zip"
  SOURCE_DIR ${CMAKE_BINARY_DIR}/External/DiscordSdk
)

FetchContent_MakeAvailable(DiscordSdk)

add_library(DiscordSdk STATIC
	${DISCORD_SDK_SRC_PATH}/achievement_manager.cpp
	${DISCORD_SDK_SRC_PATH}/achievement_manager.h
	${DISCORD_SDK_SRC_PATH}/activity_manager.cpp
	${DISCORD_SDK_SRC_PATH}/activity_manager.h
	${DISCORD_SDK_SRC_PATH}/application_manager.cpp
	${DISCORD_SDK_SRC_PATH}/application_manager.h
	${DISCORD_SDK_SRC_PATH}/core.cpp
	${DISCORD_SDK_SRC_PATH}/core.h
	${DISCORD_SDK_SRC_PATH}/discord.h
	${DISCORD_SDK_SRC_PATH}/event.h
	${DISCORD_SDK_SRC_PATH}/ffi.h
	${DISCORD_SDK_SRC_PATH}/image_manager.cpp
	${DISCORD_SDK_SRC_PATH}/image_manager.h
	${DISCORD_SDK_SRC_PATH}/lobby_manager.cpp
	${DISCORD_SDK_SRC_PATH}/lobby_manager.h
	${DISCORD_SDK_SRC_PATH}/network_manager.cpp
	${DISCORD_SDK_SRC_PATH}/network_manager.h
	${DISCORD_SDK_SRC_PATH}/overlay_manager.cpp
	${DISCORD_SDK_SRC_PATH}/overlay_manager.h
	${DISCORD_SDK_SRC_PATH}/relationship_manager.cpp
	${DISCORD_SDK_SRC_PATH}/relationship_manager.h
	${DISCORD_SDK_SRC_PATH}/storage_manager.cpp
	${DISCORD_SDK_SRC_PATH}/storage_manager.h
	${DISCORD_SDK_SRC_PATH}/store_manager.cpp
	${DISCORD_SDK_SRC_PATH}/store_manager.h
	${DISCORD_SDK_SRC_PATH}/types.cpp
	${DISCORD_SDK_SRC_PATH}/types.h
	${DISCORD_SDK_SRC_PATH}/user_manager.cpp
	${DISCORD_SDK_SRC_PATH}/user_manager.h
	${DISCORD_SDK_SRC_PATH}/voice_manager.cpp
	${DISCORD_SDK_SRC_PATH}/voice_manager.h
)

target_include_directories(DiscordSdk PUBLIC
	${DISCORD_SDK_SRC_PATH}
)

target_link_libraries(DiscordSdk PUBLIC
	${DISCORD_SDK_LIB_PATH}
)
#### End Discord Game SDK

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS
	Src/*.cpp
	Src/*.c
	Src/*.hpp
	Src/*.h
)

add_library(DiscordRichPresence SHARED
	${SRC_FILES}
)

target_include_directories(DiscordRichPresence PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Src
	${DISCORD_SDK_INCLUDE_PATH}
)

target_link_libraries(DiscordRichPresence PRIVATE
	ZHMModSDK
	DiscordSdk
)

install(TARGETS DiscordRichPresence
	RUNTIME DESTINATION bin/mods
)