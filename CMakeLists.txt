# Minimum version of CMake that I installed with cmake-3.29.8-windows-x86_64.msi
cmake_minimum_required(VERSION 3.29)

# Project name: another_world
project(another_world)

# Set CMAKE standard to 20, for compiling purposes, 17 also works
set(CMAKE_CXX_STANDARD 20)

# Set SFML directory, important to tell where SFML is located
set(SFML_DIR "C:/SFML/lib/cmake/SFML")

# Find SFML, I am using 2.6.1 exclusively, but 2.5 also works
find_package(SFML 2.6.1 COMPONENTS graphics window system REQUIRED)

# Add executable
add_executable(another_world main.cpp)

# Link SFML libraries
target_link_libraries(another_world sfml-graphics sfml-window sfml-system sfml-audio)

# Copy SFML DLLs after build
add_custom_command(TARGET another_world POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SFML_DIR}/../../../bin"
        $<TARGET_FILE_DIR:another_world>)

# Copy assets folder to the build directory
add_custom_command(TARGET another_world POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_BINARY_DIR}/assets")

# Copy the maps folder to the build directory
add_custom_command(TARGET another_world POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/maps"
        "${CMAKE_BINARY_DIR}/maps"
)