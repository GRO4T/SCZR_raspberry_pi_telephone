#include "mock_mic_generator.hpp"

#include <atomic>
#include <memory>
#include <thread>
#include <signal.h>

std::unique_ptr<MockMicGenerator> generator = nullptr;
std::atomic<bool> generate;

void signal_handler(int sig) {
  if(sig == SIGINT) {
    generate.store(false);
  }
}

void generator_thread() {
  while(generate.load()) {
    if(!generator->is_saturated())
      generator->gen();
  }
}

int main() {
  signal(SIGINT, &signal_handler);
  generator = std::make_unique<MockMicGenerator>();
  generate.store(true);

  auto th = std::thread{generator_thread};
  th.join();

  return 0;
}