cmake_minimum_required(VERSION 3.7)

project(google_balls_wiiu CXX)

# Find SDL2 using pkg-config
find_package(PkgConfig REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2IMAGE REQUIRED SDL2_image)

# Add executable
add_executable(google_balls_wiiu
    main.cpp
)

# Include directories
target_include_directories(google_balls_wiiu PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2IMAGE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(google_balls_wiiu
    ${SDL2_LIBRARIES}
    ${SDL2IMAGE_LIBRARIES}
    wut
)

# Compile options
target_compile_options(google_balls_wiiu PRIVATE -O2)

# Create RPX
wut_create_rpx(google_balls_wiiu)