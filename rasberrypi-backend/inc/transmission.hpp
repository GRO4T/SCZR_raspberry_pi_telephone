#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

#include "mic.hpp"
#include "audio.hpp"

#include <stdio.h>
#include <cstring>
#include <errno.h>
#include <stdexcept>
#include <chrono>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unordered_set>
#include <iostream>

namespace transmission {

const unsigned int kBufferSize = 128;
const uint32_t PAC1 = 0x31434150; // 826491216
const uint32_t PAC2 = 0x32434150; // 843268432

struct __attribute__((__packed__)) MicPacket {
	uint32_t id;
	int16_t data[kBufferSize];
	uint32_t crc;
};

const std::size_t kPacketSize = sizeof(MicPacket);
const std::size_t kDataSize = kPacketSize - 8;

class IPv4 {
    in_addr addr;
public:
    IPv4(const IPv4& ) = default;
    explicit IPv4(const std::string& );
    explicit IPv4(const char* );
    explicit IPv4(unsigned long int );

    const in_addr& address() const noexcept;
    operator std::string() const;
};

std::ostream& operator<<(std::ostream& os, const IPv4& ip4) {
  os << std::string(ip4);
  return os;
}

class UDPSocket {
    int sockfd;
    sockaddr_in addr;
    sockaddr_in incoming_addr;
    sockaddr_in outgoing_addr;
public:
    UDPSocket(const IPv4& addr, int port = 8080);
    ~UDPSocket();
    void setOutgoingAddr(const IPv4& ip4, int port = 8080);
    std::string getOutgoingAddr() const;
    std::string getIncomingAddr() const;
    void send(const char * buf, std::size_t bufsize);
    int receive(char * buf, std::size_t bufsize);
    int fd() const;
};

class FDSelector {
    fd_set rd_set;
    fd_set wr_set;
    std::unordered_set<int> rd_fds;
    std::unordered_set<int> wr_fds;
public:
    bool add(const Microphone& mic);
    bool addRead(const UDPSocket& socket);
    bool addWrite(const UDPSocket& socket);

    void remove(const Microphone& mic);
    void removeRead(const UDPSocket& socket);
    void removeWrite(const UDPSocket& socket);

    bool ready(const Microphone& mic);
    bool readyRead(const UDPSocket& socket);
    bool readyWrite(const UDPSocket& socket);

    bool wait(std::chrono::milliseconds ms);
    void clear();
};

class DataFromMicRetriever {
 public:
  void fetchData(MicPacket& dest_packet);
  const Microphone& getMic() const;
 private:
  Microphone mic;
  char buffer_tmp[kPacketSize];
  bool mic_synced = false;
  std::size_t synced_at = 0;
  uint32_t last_synced_id = 0;

  void handle_synced(MicPacket& dest_packet);
  void handle_desynced(MicPacket& dest_packet);
};
class DataConverter {
 public:
  static void micPacketToAudioPacket(MicPacket& src, Audio<kBufferSize>::AudioPacket& dest);
 private:
  static const uint16_t constant_compound = 1551; // (1.25/3.3)*4096
};
class DataTransmitter {
public:
    DataTransmitter();
    void transmit(std::string dumpfile);
private:
  UDPSocket socket;
  FDSelector fd_selector;
  DataFromMicRetriever data_from_mic_retriever;

  Audio<kBufferSize> audio;
  MicPacket packet_out;
  MicPacket packet_in;
};

}

#endif
