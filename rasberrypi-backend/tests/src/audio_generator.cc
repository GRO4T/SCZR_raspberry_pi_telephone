#include "audio.hpp"

#include <cmath>
#include <transmission.hpp>

constexpr char NAME[] = "/test_audio";

constexpr unsigned int NumFrames = 5012;

int main() {
  Audio<transmission::kBufferSize>::PacketDeque ptr(NAME); // BUFFER_SIZE = 128
  float t = 0.0;
  float dt = 1.0/44100.0;

  transmission::MicPacket packet_from_mic;
  Audio<transmission::kBufferSize>::AudioPacket audio_packet;
  transmission::DataFromMicRetriever data_from_mic_retriever;

  while(1) {
    {
      auto resource = ptr->lock();
      if(resource->full())
        continue;
    }

    /*
    Audio<NumFrames>::AudioPacket packet;
    for(unsigned int i = 0; i < NumFrames; ++i) {
      packet.data[i] = 0.1 * 65536 * 0.5 * (std::sin(420*t*2.0*M_PI) + 1.0);
      t += dt;
    }
     */

    data_from_mic_retriever.fetchData(packet_from_mic);
    transmission::DataConverter::micPacketToAudioPacket(packet_from_mic, audio_packet);

    auto resource = ptr->lock();
    *resource->push_front() = audio_packet;
  }

  return 0;
}