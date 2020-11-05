#include "audio.hpp"

#include <chrono>
#include <cmath>
#include <memory>
#include <thread>

int main() {
  Audio audio{44100, 1024};

  std::unique_ptr<uint16_t[]> buffer = std::make_unique<uint16_t[]>(audio.getBufferFrames());

  for(unsigned int i = 0; i < audio.getBufferFrames(); ++i) {
    float t =  (float)i / audio.getBufferFrames();
    buffer[i] = 0.5 * 65536 * 0.5 * (std::sin(1000*t*2.0*M_PI) + 1.0);
  }

  audio.play(buffer.get());
  std::cout << "Playing audio...\n";
  std::this_thread::sleep_for(std::chrono::seconds(1));
  audio.stop();
  std::cout << "Done\n";

  return 0;
}