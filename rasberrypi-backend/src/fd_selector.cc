#include "fd_selector.hpp"

bool FDSelector::add(const MicType &mic) {
  auto[it, inserted] = rd_fds.insert(mic.fd());
  return inserted;
}

bool FDSelector::add(const TCPServer &server) {
  auto[_, inserted] = rd_fds.insert(server.fd());
  return inserted;
}

bool FDSelector::addWrite(const UDPSocket &socket) {
  auto[it, inserted] = wr_fds.insert(socket.fd());
  return inserted;
}

bool FDSelector::addWrite(const TCPConnection &conn) {
  auto[_, inserted] = wr_fds.insert(conn.fd());
  return inserted;
}

bool FDSelector::addRead(const UDPSocket &socket) {
  auto[it, inserted] = rd_fds.insert(socket.fd());
  return inserted;
}

bool FDSelector::addRead(const TCPConnection &conn) {
  auto[_, inserted] = rd_fds.insert(conn.fd());
  return inserted;
}

void FDSelector::remove(const MicType &mic) {
  rd_fds.erase(mic.fd());
}

void FDSelector::remove(const TCPServer &server) {
  rd_fds.erase(server.fd());
}

void FDSelector::removeWrite(const UDPSocket &socket) {
  wr_fds.erase(socket.fd());
}

void FDSelector::removeWrite(const TCPConnection &conn) {
  wr_fds.erase(conn.fd());
}

void FDSelector::removeRead(const UDPSocket &socket) {
  rd_fds.erase(socket.fd());
}

void FDSelector::removeRead(const TCPConnection &conn) {
  rd_fds.erase(conn.fd());
}

bool FDSelector::ready(const MicType &mic) {
  return FD_ISSET(mic.fd(), &rd_set);
}

bool FDSelector::ready(const TCPServer &server) const {
  return FD_ISSET(server.fd(), &rd_set);
}

bool FDSelector::readyWrite(const UDPSocket &socket) {
  return FD_ISSET(socket.fd(), &wr_set);
}

bool FDSelector::readyWrite(const TCPConnection &conn) const {
  return FD_ISSET(conn.fd(), &wr_set);
}

bool FDSelector::readyRead(const UDPSocket &socket) {
  return FD_ISSET(socket.fd(), &rd_set);
}

bool FDSelector::readyRead(const TCPConnection &conn) const {
  return FD_ISSET(conn.fd(), &rd_set);
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
