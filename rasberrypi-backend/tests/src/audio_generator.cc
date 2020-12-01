#include "audio.hpp"
#include "config.hpp"

#include <cmath>

int main() {
  Audio<NUM_FRAMES>::PacketDeque ptr(SHM_AUDIO_TEST_NAME);
  float t = 0.0;
  float dt = 1.0/SAMPLING_RATE;

  while(1) {
    {
      auto resource = ptr->lock();
      if(resource->full())
        continue;
    }

    Audio<NUM_FRAMES>::AudioPacket packet;
    for(unsigned int i = 0; i < NUM_FRAMES; ++i) {
      packet.data[i] = 0.1 * 65536 * 0.5 * (std::sin(420*t*2.0*M_PI) + 1.0);
      t += dt;
    }

    auto resource = ptr->lock();
    *resource->push_front() = packet;
  }

  return 0;
}
