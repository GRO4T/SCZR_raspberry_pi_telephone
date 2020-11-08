#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

#include "mic.hpp"

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

const int BUFFER_SIZE = 128;

struct __attribute__((__packed__)) Packet {
	uint32_t id;
	int16_t data[BUFFER_SIZE];
	uint32_t crc;
};

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

class DataTransmitter {
    Microphone mic;
    UDPSocket socket;
    FDSelector fd_selector;
public:
    DataTransmitter();
    void transmit(std::string dumpfile);
};

#endif
