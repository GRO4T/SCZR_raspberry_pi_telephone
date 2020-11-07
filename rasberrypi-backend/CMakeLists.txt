cmake_minimum_required(VERSION 3.0)

project(telefon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(inc)
include_directories(rtaudio)
add_subdirectory(rtaudio)

add_library(phone
        inc/mic.hpp
        src/mic.cc
        inc/audio.hpp
        inc/ipc.hpp
    )

add_subdirectory(tests)

target_link_libraries(phone rtaudio rt)

add_custom_target(CopyCompileCommands ALL
        ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
