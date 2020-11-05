
add_executable(dump_mic_to_file
        src/mic_dump_to_file.cc
    )

add_executable(audio
    src/audio.cc
)

target_link_libraries(dump_mic_to_file phone)
target_link_libraries(audio phone)
