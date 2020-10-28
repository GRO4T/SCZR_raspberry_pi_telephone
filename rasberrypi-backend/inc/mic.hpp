#ifndef __MIC_HPP__
#define __MIC_HPP__

#include <cstdint>

class Microphone {
    int USB;
public:
    Microphone(const char* path = "/dev/ttyACM0");
    ~Microphone();

    std::size_t read(char* buffer, std::size_t buffer_size) const;
};

#endif
