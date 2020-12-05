#include "transmission.hpp"
#include "protocol.pb.h"

#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cassert>
#include <thread>

namespace transmission {

IPv4::IPv4(const std::string &str) : IPv4(str.c_str()) {}

IPv4::IPv4(const char *ptr) {
  if (inet_pton(AF_INET, ptr, &addr) != 1) {
    throw NetworkException("Not a proper IPv4 address");
  }
}

IPv4::IPv4(unsigned long int _addr) {
  addr.s_addr = htonl(_addr);
}

const in_addr &IPv4::address() const noexcept {
  return addr;
}

IPv4::operator std::string() const {
  char str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(addr), str, INET_ADDRSTRLEN);
  return std::string(str);
}

UDPSocket::UDPSocket(const IPv4 &ip4, int port) {
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    throw NetworkException("Socket creation failed");
  }
  int enable = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    throw NetworkException("setsockopt(SO_REUSEADDR) failed");
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip4.address().s_addr;
  addr.sin_port = htons(port);
  if (bind(sockfd, (const struct sockaddr *) &addr, sizeof(addr)) < 0) {
    throw NetworkException("Bind failed.");
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

int UDPSocket::send(const char *buf, std::size_t bufsize) {
  int bytes_sent = sendto(sockfd, buf, bufsize, MSG_CONFIRM, (const struct sockaddr *) &outgoing_addr, sizeof(outgoing_addr));
  if (bytes_sent == -1) {
    throw NetworkException("Error sending datagram");
  }
  return bytes_sent;
}

int UDPSocket::receive(char *buf, std::size_t bufsize) {
  int len = sizeof(incoming_addr);
  int bytes_received;
  bytes_received = recvfrom(sockfd, buf, bufsize, MSG_WAITALL, (struct sockaddr *) &incoming_addr, (socklen_t *) &len);
  if (bytes_received == -1) {
    throw NetworkException("Error receiving datagram");
  }
  return bytes_received;
}

int UDPSocket::fd() const {
  return sockfd;
}

bool FDSelector::add(const MicType &mic) {
  auto[it, inserted] = rd_fds.insert(mic.fd());
  return inserted;
}

bool FDSelector::addWrite(const UDPSocket &socket) {
  auto[it, inserted] = wr_fds.insert(socket.fd());
  return inserted;
}

bool FDSelector::addRead(const UDPSocket &socket) {
  auto[it, inserted] = rd_fds.insert(socket.fd());
  return inserted;
}

void FDSelector::remove(const MicType &mic) {
  rd_fds.erase(mic.fd());
}

void FDSelector::removeWrite(const UDPSocket &socket) {
  wr_fds.erase(socket.fd());
}

void FDSelector::removeRead(const UDPSocket &socket) {
  rd_fds.erase(socket.fd());
}

bool FDSelector::ready(const MicType &mic) {
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

void DataFromMicRetriever::fetchData(MicPacket &dest_packet) {
  if (!mic_synced) {
    handleDesynced(dest_packet);
  } else {
    handleSynced(dest_packet);
  }
}

void DataFromMicRetriever::handleSynced(MicPacket &dest_packet) {
  int n = mic.read(buffer_tmp, PACKET_SIZE);
  assert(n == PACKET_SIZE);
  memcpy((char *) &dest_packet.id, buffer_tmp, 4); // copy id
  if (!((dest_packet.id == PAC1 || dest_packet.id == PAC2) && (dest_packet.id != last_synced_id))) { // if id invalid
    handleDesynced(dest_packet);
    return;
  }
  last_synced_id = dest_packet.id;
  memcpy((char *) dest_packet.data, buffer_tmp + 4, PACKET_SIZE - 4); // copy rest of the packet
}

void DataFromMicRetriever::handleDesynced(MicPacket &dest_packet) {
  mic.read(buffer_tmp, PACKET_SIZE);
  // find id
  for (std::size_t i = 0; i < PACKET_SIZE; ++i) {
    uint32_t id = *reinterpret_cast<uint32_t *>(buffer_tmp + i);
    if (id == PAC1 || id == PAC2) {
      mic_synced = true;
      synced_at = i;
      last_synced_id = id;
      dest_packet.id = id;
      break;
    }
  }
  // handle rest of the packet
  std::size_t remaining_bytes = PACKET_SIZE - synced_at - 4;
  memcpy((char *) dest_packet.data, buffer_tmp + synced_at + 4, remaining_bytes);
  if (remaining_bytes < PACKET_SIZE - 4) { // more data needs to be fetched (we didn't sync at byte 0)
    const auto rest_of_the_bytes = mic.read(buffer_tmp, PACKET_SIZE - 4 - remaining_bytes);
    assert(rest_of_the_bytes == PACKET_SIZE - 4 - remaining_bytes);
    memcpy((char *) dest_packet.data + remaining_bytes, buffer_tmp, rest_of_the_bytes); // rest of the data
  }
}

const MicType &transmission::DataFromMicRetriever::getMic() const {
  return mic;
}

void DataConverter::micPacketToAudioPacket(MicPacket &src, Audio<BUFFER_SIZE>::AudioPacket &dest) {
  memcpy((char *) dest.data, (char *) src.data, DATA_SIZE);
  for (std::size_t i = 0; i < BUFFER_SIZE; ++i) {
    dest.data[i] -= constant_compound;
    dest.data[i] *= 3;
  }
}

DataTransmitter::DataTransmitter(const char* shm_name) :
      state(ConnectionState::Start),
      socket(IPv4("127.0.0.1")),
      shared_memory_deque(shm_name) {
  socket.setOutgoingAddr(IPv4("127.0.0.1"));
  fd_selector.add(data_from_mic_retriever.getMic());
  fd_selector.addRead(socket);
}

void DataTransmitter::transmit() {
  while (true) {
    fd_selector.wait(std::chrono::milliseconds(10));
    if (fd_selector.ready(data_from_mic_retriever.getMic())) {
      fetchFromMicAndSendOverNetwork();
    }
    if (fd_selector.readyRead(socket)) {
      receiveFromNetworkAndPutInSharedMemory();
    }
  }
}

void DataTransmitter::fetchFromMicAndSendOverNetwork() {
  data_from_mic_retriever.fetchData(packet_out);
  assert(packet_out.id == PAC1 || packet_out.id == PAC2);

  Request request{};
  Data* data = new Data();
  data->set_data((char*)&packet_out, PACKET_SIZE);
  request.set_allocated_data(data);

  std::string serialized_data;
  request.SerializeToString(&serialized_data);
  
  ProtocolData data_wrapped{};
  data_wrapped.size = sizeof(serialized_data);
  memcpy(&data_wrapped.serialized_data, serialized_data.data(), SERIALIZED_SIZE);

  socket.send((char*)&data_wrapped, sizeof(data_wrapped));
}

void DataTransmitter::receiveFromNetworkAndPutInSharedMemory() {
  {
    auto resource = shared_memory_deque->lock();
    if(resource->full())
      return;
  }

  ProtocolData data_wrapped{};
  socket.receive((char*)&data_wrapped.size, sizeof(data_wrapped.size));
  socket.receive((char*)&data_wrapped.serialized_data, data_wrapped.size);

  std::string serialized_data;
  serialized_data.reserve(SERIALIZED_SIZE);
  memcpy(serialized_data.data(), &data_wrapped.serialized_data, SERIALIZED_SIZE);

  Response response{};
  const auto result = response.ParseFromString(serialized_data);
  assert(result);

  if(response.Content_case() == Response::ContentCase::kData) {
    const auto& data = response.data();
    memcpy(&packet_in, &data.data(), PACKET_SIZE);
    assert(packet_in.id == PAC1 || packet_in.id == PAC2);
    DataConverter::micPacketToAudioPacket(packet_in, audio_packet);

    auto resource = shared_memory_deque->lock();
    *resource->push_front() = audio_packet;
  }
}

}