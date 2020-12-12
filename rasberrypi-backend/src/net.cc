#include "net.hpp"

#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cassert>
#include <fstream>

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

IPv6::IPv6(const std::string& str) : IPv6(str.c_str()) {  }

IPv6::IPv6(const char* ptr) {
  if (inet_pton(AF_INET6, ptr, &addr) != 1) {
    throw NetworkException();
  }
}

const in6_addr& IPv6::address() const noexcept {
  return addr;
}

std::ostream &operator<<(std::ostream &os, const IPv4 &ip4) {
  os << std::string(ip4);
  return os;
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

int UDPSocket::write(const char *buf, std::size_t bufsize) {
  int bytes_sent = sendto(sockfd, buf, bufsize, MSG_CONFIRM, (const struct sockaddr *) &outgoing_addr, sizeof(outgoing_addr));
  if (bytes_sent == -1) {
    throw NetworkException("Error sending datagram");
  }
  return bytes_sent;
}

int UDPSocket::read(char *buf, std::size_t bufsize) {
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

TCPConnection::TCPConnection(int sockfd, const struct sockaddr_in6& addr) : sockfd(sockfd), addr(addr) {}
TCPConnection::~TCPConnection() {
  close(sockfd);
}

std::string TCPConnection::address() const {
  char buffer[INET6_ADDRSTRLEN];
  if (addr.sin6_family == AF_INET) {
    struct sockaddr_in* addr_v4 = (struct sockaddr_in*)(&addr);
    if (inet_ntop(addr_v4->sin_family, &addr_v4->sin_addr, buffer, sizeof(buffer)) < 0) {
      throw NetworkException();
    }
  } else {
    if (inet_ntop(addr.sin6_family, &addr.sin6_addr, buffer, sizeof(buffer)) < 0) {
      throw NetworkException();
    }
  }

  return std::string(buffer);
}

short unsigned int TCPConnection::port() const {
  return ntohs(addr.sin6_port);
}

std::size_t TCPConnection::read(char* buffer, std::size_t buffer_size) const {
  ssize_t result = ::recv(sockfd, buffer, buffer_size, 0);
  if (result < 0) {
    throw NetworkException();
  } else if (result == 0) {
    throw NetworkException("No data received");
  } else {
    return static_cast<std::size_t>(result);
  }
}

std::size_t TCPConnection::write(const char* buffer, std::size_t buffer_size) const {
  ssize_t result = ::send(sockfd, buffer, buffer_size, 0);
  if (result < 0) {
    throw NetworkException();
  } else {
    return static_cast<std::size_t>(result);
  }
}

void TCPConnection::shutdown(int how) {
  ::shutdown(sockfd, how);
}

void TCPConnection::setsockopt(int level, int optname, const void *optval, socklen_t optlen) {
  if (::setsockopt(sockfd, level, optname, optval, optlen) < 0) {
    throw NetworkException();
  }
}

int TCPConnection::fd() const noexcept {
  return sockfd;
}

Connection::Connection(int sockfd, const struct sockaddr_in6& addr) : TCPConnection(sockfd, addr) {
  state = Connection::PacketState::Start;
  already_sent = 0;
  buffer.fill(0);
}

bool Connection::recvPayload(std::string& payload) {
  if (state == Connection::PacketState::Start) {
    std::size_t received = read(reinterpret_cast<char*>(&payload_size), sizeof(payload_size));
    if (received != sizeof(payload_size)) {
      return false;
    }
    state = Connection::PacketState::RecvOrSendingPayload;
  }
  if (state == Connection::PacketState::RecvOrSendingPayload) {
    if (payload_size == 0 || recv(payload, payload_size)) {
      state = Connection::PacketState::Start;
      return true;
    }
  }
  return false;
}

bool Connection::sendPayload(std::string& payload) {
  if (state == Connection::PacketState::Start) {
    payload_size = payload.length();
    std::size_t sent = write(reinterpret_cast<char*>(&payload_size), sizeof(payload_size));
    if (sent != sizeof(payload_size)) {
      return false;
    }
    state = Connection::PacketState::RecvOrSendingPayload;
    already_sent = 0;
  }
  if (state == Connection::PacketState::RecvOrSendingPayload) {
    if (send(payload, already_sent)) {
      state = Connection::PacketState::Start;
      return true;
    }
  }
  return false;
}

bool Connection::sendfileChunk(std::fstream& file, std::size_t file_size) {
  auto position = file.tellg();
  file.read(buffer.data(), std::min(file_size, buffer.size()));
  auto size = file.gcount();
  auto written = write(buffer.data(), size);
  auto current_position = static_cast<std::size_t>(position) + static_cast<std::size_t>(written);
  file.seekg(current_position);
  return current_position == file_size;
}

bool Connection::recvfileChunk(std::fstream& file, std::size_t file_size) {
  auto position = file.tellp();
  auto received = read(buffer.data(),
                       std::min(buffer.size(), static_cast<std::size_t>(file_size) - static_cast<std::size_t>(position))
  );
  file.write(buffer.data(), received);
  return file.tellp() == file_size;
}

void Connection::sendFile(std::fstream& file, std::size_t file_size) {
  while (!sendfileChunk(file, file_size));
}

void Connection::recvFile(std::fstream& file, std::size_t file_size) {
  while (!recvfileChunk(file, file_size));
}

bool Connection::recv(std::string& data, std::size_t end_size) {
  buffer.fill(0);
  std::size_t to_receive = end_size - data.length();
  std::size_t received = read(buffer.data(), std::min(to_receive, buffer.size()));
  data.append(buffer.data(), received);
  return data.length() == end_size;
}

bool Connection::send(const std::string& data, std::size_t& sent) {
  std::size_t to_send = data.length() - sent;
  std::size_t written = write(&data[sent], to_send);
  sent = written;
  return data.length() == sent;
}


std::unique_ptr<Connection> connect(const IPv4& ip, unsigned short port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    throw NetworkException();
  }
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr = ip.address();
  addr.sin_port = htons(port);
  if (::connect(sockfd, (const sockaddr*)&addr, sizeof(addr)) != 0) {
    ::close(sockfd);
    throw NetworkException();
  }

  struct sockaddr_in6 remote;
  socklen_t len = sizeof(remote);
  if (::getpeername(sockfd, (struct sockaddr*)&remote, &len) < 0) {
    throw NetworkException();
  }
  return std::make_unique<Connection>(sockfd, remote);
}

std::unique_ptr<Connection> connect(const IPv6& ip, unsigned short port) {
  int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
  if (sockfd < 0) {
    throw NetworkException();
  }
  sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_addr = ip.address();
  addr.sin6_port = htons(port);
  if (::connect(sockfd, (const sockaddr*)&addr, sizeof(addr)) < 0) {
    ::close(sockfd);
    throw NetworkException();
  }
  return std::make_unique<Connection>(sockfd, addr);
}

std::unique_ptr<Connection> connect(const std::string& host, const std::string& port) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  struct addrinfo* result;
  int status = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
  if (status != 0) {
    throw NetworkException(gai_strerror(status));
  }

  int sockfd = -1;
  struct addrinfo* ptr = nullptr;
  for (ptr=result; ptr; ptr = ptr->ai_next) {
    sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

    if (sockfd < 0) {
      freeaddrinfo(result);
      throw NetworkException();
    }

    if (::connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) != -1) {
      break;
    }
    close(sockfd);
  }

  freeaddrinfo(result);
  if (!ptr) {
    throw NetworkException("Connection refused");
  }

  struct sockaddr_in6 remote;
  socklen_t len = sizeof(remote);
  if (::getpeername(sockfd, (struct sockaddr*)&remote, &len) < 0) {
    throw NetworkException();
  }
  return std::make_unique<Connection>(sockfd, remote);
}

std::unique_ptr<Connection> connect(const std::string& url) {
  if (url.length() == 0) {
    throw NetworkException("Invalid address");
  }
  if (url[0] == '[') { // parsing ipv6
    auto host_end = url.find(']');
    if (host_end == std::string::npos || url.at(host_end+1) != ':') {
      throw NetworkException("Invalid address");
    }
    auto host = url.substr(1, host_end - 1);
    auto port = url.substr(host_end + 2);
    return connect(host, port);
  } else { // parsing ipv4
    auto host_end = url.find(':');
    if (host_end == std::string::npos) {
      throw NetworkException("Invalid address");
    }
    auto host = url.substr(0, host_end);
    auto port = url.substr(host_end + 1);
    return connect(host, port);
  }
}

TCPServer::TCPServer() {
  sockfd = -1;
}

TCPServer::~TCPServer() {
  if (sockfd != -1) {
    ::close(sockfd);
  }
}

void TCPServer::reuseaddr(bool reuse) {
  if (sockfd == -1) {
    throw NetworkException("Server's socket not initialized");
  }
  int option = (reuse == true ? 1 : 0);
  if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
    throw NetworkException();
  }
}

void TCPServer::bind(const IPv4& ip, unsigned short port) {
  if (sockfd == -1) {
    sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      throw NetworkException();
    }
  }
  reuseaddr(true);
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr = ip.address();
  addr.sin_port = htons(port);
  if (::bind(sockfd, (const sockaddr*)&addr, sizeof(addr)) < 0) {
    throw NetworkException();
  }
}

void TCPServer::bind(const IPv6& ip, unsigned short port) {
  if (sockfd == -1) {
    sockfd = ::socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0) {
      throw NetworkException();
    }
  }
  reuseaddr(true);
  sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_addr = ip.address();
  addr.sin6_port = htons(port);
  if (::bind(sockfd, (const sockaddr*)&addr, sizeof(addr)) < 0) {
    throw NetworkException();
  }
}

void TCPServer::listen(std::size_t n) {
  if (::listen(sockfd, n) < 0) {
    throw NetworkException();
  }
}

std::unique_ptr<Connection> TCPServer::accept() {
  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));
  socklen_t len = sizeof(addr);

  int clientfd = ::accept(sockfd, reinterpret_cast<struct sockaddr*>(&addr), &len);
  if (clientfd < 0) {
    throw NetworkException();
  }
  return std::make_unique<Connection>(clientfd, addr);
}

int TCPServer::fd() const noexcept {
  return sockfd;
}



