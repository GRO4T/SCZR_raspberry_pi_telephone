#include "transmission.hpp"

#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <unistd.h>

IPv4::IPv4(const std::string& str) : IPv4(str.c_str()) {  }

IPv4::IPv4(const char* ptr) {
    if (inet_pton(AF_INET, ptr, &addr) != 1) {
        throw std::runtime_error("Not a proper IPv4 address");
    }
}

IPv4::IPv4(unsigned long int _addr) {
    addr.s_addr = htonl(_addr);
}

const in_addr& IPv4::address() const noexcept {
    return addr;
}

IPv4::operator std::string() const {
  char str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(addr), str, INET_ADDRSTRLEN);
  return std::string(str);
}

UDPSocket::UDPSocket(const IPv4 &ip4, int port) {
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    throw std::runtime_error("Socket creation failed");
  }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip4.address().s_addr;
  addr.sin_port = htons(port);
  if (bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
    throw std::runtime_error("Bind failed");
  }
}

UDPSocket::~UDPSocket() {
  close(sockfd);
}

void UDPSocket::setOutgoingAddr(const IPv4 &ip4, int port) {
  outgoing_addr.sin_family = AF_INET;
  outgoing_addr.sin_addr.s_addr = ip4.address().s_addr;
  outgoing_addr.sin_port = htons(port);
}

std::string UDPSocket::getOutgoingAddr() const {
  return IPv4(ntohl(outgoing_addr.sin_addr.s_addr));
}

std::string UDPSocket::getIncomingAddr() const {
  return IPv4(ntohl(incoming_addr.sin_addr.s_addr));
}

void UDPSocket::send(const char * buf, std::size_t bufsize) {
  sendto(sockfd, buf, bufsize, MSG_CONFIRM, (const struct sockaddr *) &outgoing_addr, sizeof(outgoing_addr));
}

int UDPSocket::receive(char * buf, std::size_t bufsize) {
  int n, len;
  n = recvfrom(sockfd, buf, bufsize, MSG_WAITALL, (struct sockaddr *) &incoming_addr, (socklen_t *)&len);
  return n;
}

int UDPSocket::fd() const {
  return sockfd;
}

bool FDSelector::add(const Microphone &mic) {
  auto [ it, inserted ] = rd_fds.insert(mic.fd());
  return inserted;
}

bool FDSelector::addWrite(const UDPSocket &socket) {
  auto [ it, inserted ] = wr_fds.insert(socket.fd());
  return inserted;
}

bool FDSelector::addRead(const UDPSocket &socket) {
  auto [ it, inserted ] = rd_fds.insert(socket.fd());
  return inserted;
}

void FDSelector::remove(const Microphone &mic) {
  rd_fds.erase(mic.fd());
}

void FDSelector::removeWrite(const UDPSocket &socket) {
  wr_fds.erase(socket.fd());
}

void FDSelector::removeRead(const UDPSocket &socket) {
  rd_fds.erase(socket.fd());
}

bool FDSelector::ready(const Microphone &mic) {
  return FD_ISSET(mic.fd(), &rd_set);
}

bool FDSelector::readyWrite(const UDPSocket &socket) {
  return FD_ISSET(socket.fd(), &wr_set);
}

bool FDSelector::readyRead(const UDPSocket &socket) {
  return FD_ISSET(socket.fd(), &rd_set);
}

bool FDSelector::wait(std::chrono::milliseconds ms) {
  FD_ZERO(&rd_set);
  FD_ZERO(&wr_set);
  int max = 0;
  for (auto fd : rd_fds) {
    FD_SET(fd, &rd_set);
    max = std::max(max, fd);
  }
  for (auto fd : wr_fds) {
    FD_SET(fd, &wr_set);
    max = std::max(max, fd);
  }
  timeval duration;
  std::size_t sec = std::chrono::duration_cast<std::chrono::seconds>(ms).count();
  std::size_t us = std::chrono::duration_cast<std::chrono::microseconds>(ms % 1000).count();
  duration.tv_sec = sec;
  duration.tv_usec = us;
  if (::select(max + 1, &rd_set, &wr_set, nullptr, &duration) <= 0) {
    return false;
  } else {
    return true;
  }
}

void FDSelector::clear() {
  rd_fds.clear();
  wr_fds.clear();
  FD_ZERO(&rd_set);
  FD_ZERO(&wr_set);
}

DataTransmitter::DataTransmitter() : socket(IPv4("127.0.0.1")) {
    socket.setOutgoingAddr(IPv4("127.0.0.1"));
    fd_selector.add(mic);
    fd_selector.addRead(socket);
}

void DataTransmitter::transmit(std::string dumpfile) {
    std::fstream file(dumpfile, std::ios::out | std::ios::binary);
    const int MAXLINE = 1024;
    while (true) {
        char buffer_out[MAXLINE];
        char buffer_in[MAXLINE];
        fd_selector.wait(std::chrono::milliseconds(10));
        if (fd_selector.ready(mic)) {
            int n = mic.read(buffer_out, MAXLINE);
            std::cout << "Data from mic read (" << n << "bytes)..." << std::endl;
            socket.send(buffer_out, n);
            std::cout << "Data sent to " << socket.getOutgoingAddr() << "..." << std::endl;
        } 
        if (fd_selector.readyRead(socket)) {
            // n = recvfrom(net_in_fd, buffer_in, MAXLINE, MSG_WAITALL, (struct sockaddr *) &addr_out, (socklen_t *)&len);
            int n = socket.receive(buffer_in, MAXLINE);
            std::cout << "Data received from " << socket.getIncomingAddr() << " (" << n << "bytes)..." << std::endl;
            file.write(buffer_in, n);
            std::cout << "Data dumped to file..." << std::endl;
        }
    }
}

