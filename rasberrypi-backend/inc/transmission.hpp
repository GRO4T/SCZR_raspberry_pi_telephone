#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

#include "mic.hpp"
#include "config.hpp"
#include "mock_mic.hpp"
#include "audio.hpp"
#include "fd_selector.hpp"
#include "net.hpp"

#include <stdio.h>
#include <cstring>
#include <errno.h>
#include <stdexcept>
#include <iostream>

namespace transmission {

struct __attribute__((__packed__)) MicPacket {
  uint32_t id;
  int16_t data[BUFFER_SIZE];
  uint32_t crc;
};

class DataFromMicRetriever {
public:
    void fetchData(MicPacket &dest_packet);
    const MicType &getMic() const;
private:
    MicType mic;
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
};

using ConnPtr = std::unique_ptr<Connection>;

class DataTransmitter {
public:
    enum class Mode {
        SEND_RECEIVE,
        RECEIVE_ONLY,
        SEND_ONLY
    };

    DataTransmitter(const char *shm_name, ConnPtr conn);
    void transmit();
    void setMode(Mode mode);

private:
    Mode mode;

    ConnPtr conn;
    FDSelector fd_selector;

    DataFromMicRetriever data_from_mic_retriever;
    MicPacket packet_out;
    MicPacket packet_in;

    Audio<BUFFER_SIZE>::AudioPacket audio_packet;
    Audio<BUFFER_SIZE>::PacketDeque shared_memory_deque;

    void fetchFromMicAndSendOverNetwork();
    void receiveFromNetworkAndPutInSharedMemory();
};

} // namespace

#endif
