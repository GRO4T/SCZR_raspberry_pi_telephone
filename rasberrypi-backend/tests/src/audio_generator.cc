#include "audio.hpp"

#include <cmath>

constexpr char NAME[] = "/test_audio";

constexpr unsigned int NumFrames = 5012;

int main() {
  Audio<NumFrames>::PacketDeque ptr(NAME);
  float t = 0.0;
  float dt = 1.0/44100.0;

  while(1) {
    {
      auto resource = ptr->lock();
      if(resource->full())
        continue;
    }

    Audio<NumFrames>::AudioPacket packet;
    for(unsigned int i = 0; i < NumFrames; ++i) {
      packet.data[i] = 0.1 * 65536 * 0.5 * (std::sin(420*t*2.0*M_PI) + 1.0);
      t += dt;
    }

    auto resource = ptr->lock();
    *resource->push_front() = packet;
  }

  return 0;
}