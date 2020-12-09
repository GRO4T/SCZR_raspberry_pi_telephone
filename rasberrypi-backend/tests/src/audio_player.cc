#include "audio.hpp"
#include "config.hpp"

int main() {
  Audio<BUFFER_SIZE> audio{SAMPLING_RATE};
  Audio<BUFFER_SIZE>::PacketDeque ptr(SHM_AUDIO_TEST_NAME);

  audio.play(ptr);
  while(1) {}

  return 0;
}