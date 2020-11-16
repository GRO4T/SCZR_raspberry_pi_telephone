#include "transmission.hpp"

#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cassert>
#include <thread>

transmission::IPv4::IPv4(const std::string& str) : IPv4(str.c_str()) {  }

transmission::IPv4::IPv4(const char* ptr) {
    if (inet_pton(AF_INET, ptr, &addr) != 1) {
        throw std::runtime_error("Not a proper IPv4 address");
    }
}

transmission::IPv4::IPv4(unsigned long int _addr) {
    addr.s_addr = htonl(_addr);
}

const in_addr& transmission::IPv4::address() const noexcept {
    return addr;
}

transmission::IPv4::operator std::string() const {
  char str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(addr), str, INET_ADDRSTRLEN);
  return std::string(str);
}

transmission::UDPSocket::UDPSocket(const IPv4 &ip4, int port) {
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    throw std::runtime_error("Socket creation failed");
  }
  int enable = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip4.address().s_addr;
  addr.sin_port = htons(port);
  if (bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
    throw std::runtime_error("Bind failed");
  }
}

transmission::UDPSocket::~UDPSocket() {
  close(sockfd);
}

void transmission::UDPSocket::setOutgoingAddr(const IPv4 &ip4, int port) {
  outgoing_addr.sin_family = AF_INET;
  outgoing_addr.sin_addr.s_addr = ip4.address().s_addr;
  outgoing_addr.sin_port = htons(port);
}

std::string transmission::UDPSocket::getOutgoingAddr() const {
  return IPv4(ntohl(outgoing_addr.sin_addr.s_addr));
}

std::string transmission::UDPSocket::getIncomingAddr() const {
  return IPv4(ntohl(incoming_addr.sin_addr.s_addr));
}

void transmission::UDPSocket::send(const char * buf, std::size_t bufsize) {
  sendto(sockfd, buf, bufsize, MSG_CONFIRM, (const struct sockaddr *) &outgoing_addr, sizeof(outgoing_addr));
}

int transmission::UDPSocket::receive(char * buf, std::size_t bufsize) {
  int n, len;
  n = recvfrom(sockfd, buf, bufsize, MSG_WAITALL, (struct sockaddr *) &incoming_addr, (socklen_t *)&len);
  return n;
}

int transmission::UDPSocket::fd() const {
  return sockfd;
}

bool transmission::FDSelector::add(const Microphone &mic) {
  auto [ it, inserted ] = rd_fds.insert(mic.fd());
  return inserted;
}

bool transmission::FDSelector::addWrite(const UDPSocket &socket) {
  auto [ it, inserted ] = wr_fds.insert(socket.fd());
  return inserted;
}

bool transmission::FDSelector::addRead(const UDPSocket &socket) {
  auto [ it, inserted ] = rd_fds.insert(socket.fd());
  return inserted;
}

void transmission::FDSelector::remove(const Microphone &mic) {
  rd_fds.erase(mic.fd());
}

void transmission::FDSelector::removeWrite(const UDPSocket &socket) {
  wr_fds.erase(socket.fd());
}

void transmission::FDSelector::removeRead(const UDPSocket &socket) {
  rd_fds.erase(socket.fd());
}

bool transmission::FDSelector::ready(const Microphone &mic) {
  return FD_ISSET(mic.fd(), &rd_set);
}

bool transmission::FDSelector::readyWrite(const UDPSocket &socket) {
  return FD_ISSET(socket.fd(), &wr_set);
}

bool transmission::FDSelector::readyRead(const UDPSocket &socket) {
  return FD_ISSET(socket.fd(), &rd_set);
}

bool transmission::FDSelector::wait(std::chrono::milliseconds ms) {
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

void transmission::FDSelector::clear() {
  rd_fds.clear();
  wr_fds.clear();
  FD_ZERO(&rd_set);
  FD_ZERO(&wr_set);
}

void transmission::DataFromMicRetriever::fetchData(MicPacket &dest_packet) {
  int n = mic.read(buffer_tmp, kPacketSize);
  if (!mic_synced) {
    for (std::size_t i = 0; i < kPacketSize; ++i) {
      uint32_t id = *reinterpret_cast<uint32_t *>(buffer_tmp + i);
      if (id == PAC1 || id == PAC2) {
        mic_synced = true;
        synced_at = i;
        dest_packet.id = id;
        std::size_t data_chunk_size = std::min(kPacketSize - synced_at - 4, std::size_t(kDataSize));
        memcpy(dest_packet.data, buffer_tmp + synced_at + 4, data_chunk_size); // first portion of data
        if (data_chunk_size == kDataSize) { // synced at byte 0
          memcpy(&dest_packet.crc, buffer_tmp + kDataSize + 4, 4); // crc
        }
        else { // fetch rest of the packet
          int n = mic.read(buffer_tmp, kPacketSize - 4 - data_chunk_size);
          assert(n == kPacketSize - 4 - data_chunk_size);
          memcpy(dest_packet.data + data_chunk_size, buffer_tmp, n - 4); // rest of the data
          memcpy(dest_packet.data + data_chunk_size + n - 4, buffer_tmp + n - 4, 4); // crc
        }
        break;
      }
    }
  }
  else {
    uint32_t old_id = dest_packet.id;
    assert(n == kPacketSize);
    memcpy(&dest_packet.id, buffer_tmp, 4);
    assert(dest_packet.id == PAC1 || dest_packet.id == PAC2);
    assert(dest_packet.id != old_id);
    memcpy(dest_packet.data, buffer_tmp + 4, kDataSize);
    memcpy(&dest_packet.crc, buffer_tmp + 4 + kDataSize, 4);
  }
}
const Microphone& transmission::DataFromMicRetriever::getMic() const {
  return mic;
}

void transmission::DataConverter::micPacketToAudioPacket(MicPacket &src, Audio<kBufferSize>::AudioPacket &dest) {
  memcpy(src.data, dest.data, kBufferSize);
}

transmission::DataTransmitter::DataTransmitter() : socket(IPv4("127.0.0.1")), audio(44100) {
  socket.setOutgoingAddr(IPv4("127.0.0.1"));
  fd_selector.add(data_from_mic_retriever.getMic());
  fd_selector.addRead(socket);
}

void transmission::DataTransmitter::transmit(std::string dumpfile) {
  std::fstream file(dumpfile, std::ios::out | std::ios::binary);
  while (true) {
    fd_selector.wait(std::chrono::milliseconds(10));
    if (fd_selector.ready(data_from_mic_retriever.getMic())) {
      data_from_mic_retriever.fetchData(packet_out);
      assert(packet_out.id == PAC1 || packet_out.id == PAC2);
      socket.send((char*)&packet_out, kPacketSize);
    }
    if (fd_selector.readyRead(socket)) {
      socket.receive((char*)&packet_in, kPacketSize);
      assert(packet_in.id == PAC1 || packet_in.id == PAC2);
      //audio.play((uint16_t*)packet_in.data);
      //std::this_thread::sleep_for(std::chrono::milliseconds(250));
      //audio.stop();
    }
  }
}
