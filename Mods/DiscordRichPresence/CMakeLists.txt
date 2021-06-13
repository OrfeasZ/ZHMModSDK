cmake_minimum_required(VERSION 3.12)

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
	${CMAKE_SOURCE_DIR}/External/discord-rpc/include
)

target_link_libraries(DiscordRpc PRIVATE
	ZHMModSDK
	discord-rpc
)
