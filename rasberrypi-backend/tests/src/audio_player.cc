#include "audio.hpp"

constexpr char NAME[] = "/test_audio";

constexpr unsigned int NumFrames = 5012;

int main() {
  Audio<NumFrames> audio{44100};
  Audio<NumFrames>::PacketDeque ptr(NAME);

  audio.play(ptr);
  while(1) {}
  audio.stop();

  return 0;
}