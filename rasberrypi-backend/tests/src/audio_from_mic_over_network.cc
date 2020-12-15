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
  while (1) {}
}

void transmitDataFromMicOverNetwork() {
  IPv4 ip("127.0.0.1");
  int port = 8081;
  TCPServer srv;
  srv.bind(ip, port);
  srv.reuseaddr();
  srv.listen();
  auto conn = srv.accept();
  transmission::DataTransmitter data_transmitter(SHM_AUDIO_TEST_NAME, std::move(conn));
  data_transmitter.setMode(transmission::DataTransmitter::Mode::SEND_ONLY);
  data_transmitter.transmit();
}

void receiveDataFromNetworkAndPutInSharedMemory() {
  IPv4 ip("127.0.0.1");
  int port = 8081;
  auto client = connect(ip, port);
  transmission::DataTransmitter data_transmitter(SHM_AUDIO_TEST_NAME, std::move(client));
  data_transmitter.setMode(transmission::DataTransmitter::Mode::RECEIVE_ONLY);
  data_transmitter.transmit();
}

int forkAndExecute(std::function<void()> func) {
  pid_t child_pid = fork();
  if (child_pid < 0) {
    throw std::runtime_error("Fork failed.");
  } else if (child_pid == 0) {
    func();
  }
  return child_pid;
}

int main() {
  int pid = forkAndExecute(playAudio);
  if (pid > 0) {
    pid = forkAndExecute(transmitDataFromMicOverNetwork);
    if (pid > 0) {
      pid = forkAndExecute(receiveDataFromNetworkAndPutInSharedMemory);
    }
  }
  if (pid > 0) {
    wait(NULL);
  }
  return 0;
}
