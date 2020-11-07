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
};

class UDPSocket {
    int sockfd;
    sockaddr_in addr_in;
    sockaddr_in addr_out;
public:
    UDPSocket(const IPv4& addr, int port = 8080);
    void send(std::string msg);
    std::string receive(std::size_t n);
};

class DataTransmitter {
    fd_set rd_set;
    fd_set wr_set;
    int mic_fd;
    int net_out_fd;
    int net_in_fd;

    Microphone mic;

// For testing purposes
    static const int PORT = 8080; 
    static const int MAXLINE = 1024;
    struct sockaddr_in addr_in;
    struct sockaddr_in addr_out;

    bool waitForData(std::chrono::milliseconds ms);
    bool micReady() const;
    bool netOutReady() const;
    bool netInReady() const;
public:
    DataTransmitter();
    void transmit(std::string dumpfile);
};

#endif
