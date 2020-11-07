#include "audio.hpp"

#include <chrono>
#include <cmath>
#include <memory>
#include <thread>

constexpr char NAME[] = "/test_audio";

#define TTT 5012

int main() {
  Audio<TTT> audio{44100};

  shared_mem_ptr<fast_deque<Audio<TTT>::AudioPacket, 1024>> ptr(NAME);

  audio.play(ptr);
  while(1) {}
  audio.stop();

  return 0;
}