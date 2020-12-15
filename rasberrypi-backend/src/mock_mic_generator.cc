#include "mock_mic_generator.hpp"
#include "config.hpp"
#include "error.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cmath>
#include <iostream>

MockMicGenerator::MockMicGenerator(const char *path) {
  time_ = 0.0f;
  if (mkfifo(path, 0666) < 0) {
    throw std::runtime_error("Could not create FIFO!");
  }
  fd_ = open(path, O_RDWR);
  if (fd_ < 0)
    throw BackendException();
  path_ = path;
}

MockMicGenerator::~MockMicGenerator() {
  close(fd_);
  remove(path_.c_str());
}

bool MockMicGenerator::is_saturated() const {
  int cap = fcntl(fd_, F_GETPIPE_SZ);
  int size = 0;
  ioctl(fd_, FIONREAD, &size);

  size_t remaining = cap - size;
  return remaining < 10 * PACKET_SIZE;
}

void MockMicGenerator::gen() {
  int16_t buf[BUFFER_SIZE];
  const uint32_t mock_crc = 0xDEADBEEF;
  const float dt = 1.0 / 44100.0;

  for (unsigned int i = 0; i < BUFFER_SIZE; ++i) {
    buf[i] = std::min(constant_compound + constant_compound * 0.2 * std::sin(420 * time_ * 2.0 * M_PI), 4096.0);
    time_ += dt;
  }

  write(fd_, &PAC1, 4);
  write(fd_, &buf, sizeof(buf));
  write(fd_, &mock_crc, 4);
}

int MockMicGenerator::fd() const {
  return fd_;
}

