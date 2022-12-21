cmake_minimum_required(VERSION 3.12)

add_library(lz4 STATIC
    source/lz4.c
    source/lz4.h
)

target_include_directories(lz4 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)