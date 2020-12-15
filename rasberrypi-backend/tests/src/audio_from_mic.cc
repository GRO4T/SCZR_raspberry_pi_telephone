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

void fetchDataFromMic() {
  Audio<BUFFER_SIZE>::PacketDeque ptr(SHM_AUDIO_TEST_NAME);
  Audio<BUFFER_SIZE>::AudioPacket audio_packet;
  transmission::MicPacket packet_from_mic;
  transmission::DataFromMicRetriever data_from_mic_retriever;

  while (1) {
    {
      auto resource = ptr->lock();
      if (resource->full())
        continue;
    }

    data_from_mic_retriever.fetchData(packet_from_mic);
    transmission::DataConverter::micPacketToAudioPacket(packet_from_mic, audio_packet);

    auto resource = ptr->lock();
    *resource->push_front() = audio_packet;
  }
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
  if (pid > 0)
    pid = forkAndExecute(fetchDataFromMic);
  if (pid > 0) {
    wait(NULL);
  }
  return 0;
}