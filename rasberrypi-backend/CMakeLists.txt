cmake_minimum_required(VERSION 3.0)

project(telefon CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(inc)

add_library(phone
        inc/mic.hpp
        src/mic.cc
    )

add_subdirectory(tests)

add_custom_target(CopyCompileCommands ALL
        ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
