#ifndef __MOCK_MIC_GENERATOR_HPP__
#define __MOCK_MIC_GENERATOR_HPP__

#include <cstdint>
#include <string>

class MockMicGenerator {
public:
    MockMicGenerator(const char *path = "/tmp/mock_mic");
    ~MockMicGenerator();

    bool is_saturated() const;

    void gen();
    int fd() const;
private:
    float time_;
    int fd_;
    std::string path_;
};

#endif
