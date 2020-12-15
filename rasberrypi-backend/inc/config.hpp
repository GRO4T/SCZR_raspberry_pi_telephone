#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <cstdint>

//#define I_DONT_HAVE_HARDWARE

const unsigned int DEQUE_SIZE = 1024;

const unsigned int BUFFER_SIZE = 1024;
const uint32_t PACKET_SIZE = BUFFER_SIZE * 2 + 8;
const std::size_t DATA_SIZE = PACKET_SIZE - 8;

const uint32_t PAC1 = 0x31434150; // 826491216
const uint32_t PAC2 = 0x32434150; // 843268432

const uint16_t constant_compound = 1551; // (1.25/3.3)*4096

const char SHM_AUDIO_TEST_NAME[] = "/test_audio";
const unsigned int NUM_FRAMES = 5012;
const unsigned int SAMPLING_RATE = 20000;

#endif
