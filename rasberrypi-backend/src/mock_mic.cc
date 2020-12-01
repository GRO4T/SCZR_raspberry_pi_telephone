#include "mock_mic.hpp"
#include "error.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

MockMic::MockMic(const char* path) {
  fd_ = open(path, O_RDWR);
  if(fd_ < 0)
    throw BackendException();
}

MockMic::~MockMic() {
  close(fd_);
}

std::size_t MockMic::read(char* buffer, std::size_t buffer_size) const {
  std::size_t already_read = 0;
  while (already_read < buffer_size) {
      ssize_t currently_read = ::read(fd_, &buffer[already_read], buffer_size - already_read);
      if (currently_read < 0)
          throw BackendException();
      else if (currently_read == 0)
          return already_read;
      already_read += currently_read;
  }
  return already_read;
}

int MockMic::fd() const {
  return fd_;
}

