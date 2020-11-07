
add_executable(dump_mic_to_file
        src/mic_dump_to_file.cc
    )

add_executable(audio_generator
    src/audio_generator.cc
)

add_executable(audio_player
    src/audio_player.cc
)

add_executable(ipc
    src/ipc.cc
)

add_executable(deque
    src/deque_test.cc
)

add_executable(spin_lock
    src/spin_lock.cc
)

target_link_libraries(dump_mic_to_file phone)
target_link_libraries(audio_generator phone)
target_link_libraries(audio_player phone)
target_link_libraries(ipc phone)
target_link_libraries(deque phone)
target_link_libraries(spin_lock phone)
