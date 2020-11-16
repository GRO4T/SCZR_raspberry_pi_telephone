#include "audio.hpp"
#include "transmission.hpp"

constexpr char NAME[] = "/test_audio";

constexpr unsigned int NumFrames = 5012;

int main() {
  Audio<transmission::kBufferSize> audio{44100};
  Audio<transmission::kBufferSize>::PacketDeque ptr(NAME);

  audio.play(ptr);
  while(1) {}
  audio.stop();

  return 0;
}