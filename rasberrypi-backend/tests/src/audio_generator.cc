#include "audio.hpp"

#include <chrono>
#include <cmath>
#include <memory>
#include <thread>

constexpr char NAME[] = "/test_audio";

#define TTT 5012

int main() {
  shared_mem_ptr<fast_deque<Audio<TTT>::AudioPacket, 1024>> ptr(NAME);
  float t = 0.0;
  float dt = 1.0/44100.0;

  while(1) {
    if(ptr->full())
      continue;

    Audio<TTT>::AudioPacket packet;
    for(unsigned int i = 0; i < TTT; ++i) {
      packet.data[i] = 0.1 * 65536 * 0.5 * (std::sin(420*t*2.0*M_PI) + 1.0);
      t += dt;
    }
    *ptr->push_front() = packet;
  }

  return 0;
}