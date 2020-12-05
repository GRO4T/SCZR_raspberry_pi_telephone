#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

#include "mic.hpp"
#include "audio.hpp"
#include "fd_selector.hpp"
#include "net.hpp"

#include <stdio.h>
#include <cstring>
#include <errno.h>
#include <stdexcept>
#include <iostream>

namespace transmission {

const unsigned int BUFFER_SIZE = 128;
const uint32_t PAC1 = 0x31434150; // 826491216
const uint32_t PAC2 = 0x32434150; // 843268432

struct __attribute__((__packed__)) MicPacket {
  uint32_t id;
  int16_t data[BUFFER_SIZE];
  uint32_t crc;
};

const std::size_t PACKET_SIZE = sizeof(MicPacket);
const std::size_t DATA_SIZE = PACKET_SIZE - 8;

class DataFromMicRetriever {
public:
    void fetchData(MicPacket &dest_packet);
    const Microphone &getMic() const;
private:
    Microphone mic;
    char buffer_tmp[PACKET_SIZE];
    bool mic_synced = false;
    std::size_t synced_at = 0;
    uint32_t last_synced_id = 0;

    void handleSynced(MicPacket &dest_packet);
    void handleDesynced(MicPacket &dest_packet);
};
class DataConverter {
public:
    static void micPacketToAudioPacket(MicPacket &src, Audio<BUFFER_SIZE>::AudioPacket &dest);
private:
    static const uint16_t constant_compound = 1551; // (1.25/3.3)*4096
};

class DataTransmitter {
public:
    DataTransmitter(const char *shm_name);
    void transmit();
private:
    UDPSocket socket;
    FDSelector fd_selector;

    DataFromMicRetriever data_from_mic_retriever;
    MicPacket packet_out;
    MicPacket packet_in;

    Audio<transmission::BUFFER_SIZE>::AudioPacket audio_packet;
    Audio<transmission::BUFFER_SIZE>::PacketDeque shared_memory_deque;

    void fetchFromMicAndSendOverNetwork();
    void receiveFromNetworkAndPutInSharedMemory();
};

}

#endif
