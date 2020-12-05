#include "audio.hpp"
#include "config.hpp"

int main() {
  Audio<NUM_FRAMES> audio{SAMPLING_RATE};
  Audio<NUM_FRAMES>::PacketDeque ptr(SHM_AUDIO_TEST_NAME);

  audio.play(ptr);
  while(1) {}
  audio.stop();

  return 0;
}