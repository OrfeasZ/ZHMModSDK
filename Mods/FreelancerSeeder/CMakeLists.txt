cmake_minimum_required(VERSION 3.15)

file(GLOB_RECURSE SRC_FILES
        CONFIGURE_DEPENDS
        src/*.cpp
        src/*.c
        src/*.hpp
        src/*.h
)

add_library(FreelancerSeeder SHARED
        ${SRC_FILES}
)

target_include_directories(FreelancerSeeder PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_link_libraries(FreelancerSeeder PRIVATE
        ZHMModSDK
)
