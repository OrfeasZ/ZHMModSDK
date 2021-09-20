cmake_minimum_required(VERSION 3.12)

#### Begin Discord Game SDK
include(ExternalProject)

ExternalProject_Add(discord_sdk
	URL "https://dl-game-sdk.discordapp.net/2.5.6/discord_game_sdk.zip"
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	BUILD_ALWAYS OFF
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/External/DiscordSDK
)

#### End Discord Game SDK

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS
	Src/*.cpp
	Src/*.c
	Src/*.hpp
	Src/*.h
)

add_library(DiscordRpc SHARED
	${SRC_FILES}
)

target_include_directories(DiscordRpc PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Src
	${CMAKE_SOURCE_DIR}/External/DiscordSDK/cpp
)

target_link_libraries(DiscordRpc PRIVATE
	ZHMModSDK
)
