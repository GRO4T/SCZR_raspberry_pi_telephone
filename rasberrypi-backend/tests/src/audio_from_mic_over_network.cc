#include "audio.hpp"
#include "config.hpp"
#include <transmission.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <functional>

void playAudio() {
  Audio<BUFFER_SIZE> audio{SAMPLING_RATE};
  Audio<BUFFER_SIZE>::PacketDeque ptr(SHM_AUDIO_TEST_NAME);

  audio.play(ptr);
  while(1) {}
}

void transmitDataFromMicOverNetwork() {
  //transmission::DataTransmitter data_transmitter(SHM_AUDIO_TEST_NAME);
  //data_transmitter.transmit();
}

int forkAndExecute(std::function<void()> func) {
  pid_t child_pid = fork();
  if (child_pid < 0) {
    throw std::runtime_error("Fork failed.");
  }
  else if (child_pid == 0) {
    func();
  }
  return child_pid;
}

int main() {
  int pid = forkAndExecute(playAudio);
  if (pid > 0)
    pid = forkAndExecute(transmitDataFromMicOverNetwork);
  if (pid > 0) {
    wait(NULL);
  }
  return 0;
}
