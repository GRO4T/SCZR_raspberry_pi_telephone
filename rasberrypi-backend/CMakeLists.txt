cmake_minimum_required(VERSION 3.0)

project(telefon CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(protocol)
include_directories(inc ${PROTOCOL_INC})
include_directories(rtaudio)
add_subdirectory(rtaudio)

add_library(phone
        inc/mic.hpp
        src/mic.cc
        inc/mock_mic.hpp
        src/mock_mic.cc
        inc/mock_mic_generator.hpp
        src/mock_mic_generator.cc
        inc/audio.hpp
        inc/ipc.hpp
        src/ipc.cc
        inc/transmission.hpp
        src/transmission.cc
        inc/fd_selector.hpp
        src/fd_selector.cc
        inc/net.hpp
        src/net.cc
    )

add_subdirectory(tests)

target_link_libraries(phone rtaudio rt protocol)

add_executable(proces1
    src/proces1.cc
)

add_executable(proces2
    src/proces2.cc
)

target_link_libraries(proces1 phone)
target_link_libraries(proces2 phone)

add_custom_target(CopyCompileCommands ALL
        ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
