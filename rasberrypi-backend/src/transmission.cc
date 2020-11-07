#include "transmission.hpp"

#include <iostream>
#include <fstream>
#include <arpa/inet.h>

IPv4::IPv4(const std::string& str) : IPv4(str.c_str()) {  }

IPv4::IPv4(const char* ptr) {
    if (inet_pton(AF_INET, ptr, &addr) != 1) {
        // throw NetworkException();
        throw std::runtime_error("Not a proper IPv4 address");
    }
}

IPv4::IPv4(unsigned long int _addr) {
    addr.s_addr = htonl(_addr);
}

const in_addr& IPv4::address() const noexcept {
    return addr;
}

DataTransmitter::DataTransmitter() {
    if ( (net_in_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
	} 
    if ( (net_out_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
	} 

    memset(&addr_in, 0, sizeof(addr_in));
    memset(&addr_out, 0, sizeof(addr_out));

    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = INADDR_ANY;
    addr_in.sin_port = htons(PORT);

    addr_out.sin_family = AF_INET;
    addr_out.sin_addr.s_addr = INADDR_ANY;
    addr_out.sin_port = htons(PORT);

    if ( bind(net_in_fd, (const struct sockaddr *)&addr_in,  
            sizeof(addr_in)) < 0 ) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
}

// it may be really dumb, beware
void DataTransmitter::transmit(std::string dumpfile) {
    std::fstream file(dumpfile, std::ios::out | std::ios::binary);
    int n;
    int len = sizeof(addr_in);
    while (true) {
        char buffer_in[MAXLINE];
        char buffer_out[MAXLINE];
        mic.read(buffer_out, sizeof(buffer_out));
        std::cout << "Data from mic read..." << std::endl;
        sendto(net_out_fd, buffer_out, sizeof(buffer_out), MSG_CONFIRM, (const struct sockaddr *) &addr_out, sizeof(addr_in));
        std::cout << "Data sent over network..." << std::endl;
        n = recvfrom(net_in_fd, buffer_in, MAXLINE, MSG_WAITALL, (struct sockaddr *) &addr_in, (socklen_t *)&len);
        std::cout << "Data received..." << std::endl;
        buffer_in[n] = '\0';
        file.write(buffer_in, sizeof(buffer_in));
        std::cout << "Data dumped in file..." << std::endl;

        // To nie działa

        // waitForData(std::chrono::milliseconds(10));
        /*
        if (micReady()) {
            mic.read(buffer_out, sizeof(buffer_out));
            std::cout << "Data from mic read..." << std::endl;
            sendto(net_out_fd, buffer_out, sizeof(buffer_out), MSG_CONFIRM, (const struct sockaddr *) &addr_in, sizeof(addr_in));
            std::cout << "Data sent over network..." << std::endl;
        } 
        if (netInReady()) {
            n = recvfrom(net_in_fd, buffer_in, MAXLINE, MSG_WAITALL, (struct sockaddr *) &addr_out, (socklen_t *)&len);
            std::cout << "Data received..." << std::endl;
            buffer_in[n] = '\0';
            file.write(buffer_in, sizeof(buffer_in));
            std::cout << "Data dumped in file..." << std::endl;
        }
        */
    }
}

bool DataTransmitter::waitForData(std::chrono::milliseconds ms) {
    FD_ZERO(&rd_set);
    FD_ZERO(&wr_set);
    FD_SET(mic_fd, &rd_set);
    FD_SET(net_in_fd, &rd_set);
    FD_SET(net_out_fd, &wr_set);
    timeval duration;
    std::size_t sec = std::chrono::duration_cast<std::chrono::seconds>(ms).count();
    std::size_t us = std::chrono::duration_cast<std::chrono::microseconds>(ms % 1000).count();
    duration.tv_sec = sec;
    duration.tv_usec = us;
    if (select(4, &rd_set, &wr_set, nullptr, &duration) <= 0) {
        return false;
    }
    else {
        return true;
    }
}

bool DataTransmitter::micReady() const {
    return FD_ISSET(mic_fd, &rd_set);
}

bool DataTransmitter::netOutReady() const {
    return FD_ISSET(net_out_fd, &wr_set);
}

bool DataTransmitter::netInReady() const {
    return FD_ISSET(net_in_fd, &rd_set);
}

