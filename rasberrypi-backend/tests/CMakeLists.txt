
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

add_executable(network_udp
        src/network_udp.cc
)

add_executable(network_tcp
        src/network_tcp.cc
        )

add_executable(sem
    src/sem.cc
)

add_executable(audio_from_mic
        src/audio_from_mic.cc
)

add_executable(audio_from_mic_over_network
       src/audio_from_mic_over_network.cc
)

add_executable(mock_mic_gen
       src/mock_mic_generator.cc
)

target_link_libraries(dump_mic_to_file phone)
target_link_libraries(audio_generator phone)
target_link_libraries(audio_player phone)
target_link_libraries(ipc phone)
target_link_libraries(deque phone)
target_link_libraries(spin_lock phone)
target_link_libraries(network_udp phone)
target_link_libraries(network_tcp phone)
target_link_libraries(sem phone)
target_link_libraries(audio_from_mic phone)
target_link_libraries(audio_from_mic_over_network phone)
target_link_libraries(mock_mic_gen phone)
