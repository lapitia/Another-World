cmake_minimum_required(VERSION 3.29)

project(another_world)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    set(SFML_DIR "C:/SFML/lib/cmake/SFML")
endif()

find_package(SFML 2.6.1 COMPONENTS graphics window system audio REQUIRED)
find_package(OpenGL REQUIRED)

# Add executable
add_executable(another_world main.cpp)

# Link SFML
target_link_libraries(another_world
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
)

if (WIN32)
    add_custom_command(
            TARGET another_world POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SFML_DIR}/../../../bin"
            $<TARGET_FILE_DIR:another_world>
    )
else()
    add_custom_command(
            TARGET another_world POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/rainyhearts.ttf
            ${CMAKE_CURRENT_BINARY_DIR}/rainyhearts.ttf)
endif()

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
