#ifndef __MOCK_MIC_HPP__
#define __MOCK_MIC_HPP__

#include <cstdint>
#include <string>

class MockMic {
public:
  MockMic(const char* path = "/tmp/mock_mic");
  ~MockMic();

  std::size_t read(char* buffer, std::size_t buffer_size) const;
  int fd() const;
private:
  int fd_;
};

#endif
