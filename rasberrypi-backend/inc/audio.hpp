#ifndef __AUDIO_HPP__
#define __AUDIO_HPP__

#include <cstring>
#include <optional>

#include "RtAudio.h"
#include "config.hpp"
#include "ipc.hpp"

template <unsigned int frames = 64u>
class Audio {
public:
  struct AudioPacket {
    uint16_t data[frames];
  };

  using PacketDeque = shared_mem_ptr<spin_locked_resource<fast_deque<AudioPacket, DEQUE_SIZE>>>;
private:
  RtAudio dac_;
  RtAudio::StreamParameters params_;
  unsigned int sampleRate_;
  unsigned int bufferFrames_;
  std::optional<PacketDeque> buffer_;
public:
  Audio(unsigned int sampleRate);
  ~Audio();
  void play(PacketDeque ptr);
  void stop();

  unsigned int getSampleRate() const { return sampleRate_; }
  unsigned int getBufferFrames() const { return bufferFrames_; }

  static int transfer(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData);
};

template <unsigned int frames>
Audio<frames>::Audio(unsigned int sampleRate):
  dac_(), params_(), sampleRate_(sampleRate), bufferFrames_(frames), buffer_() {
  
  if(dac_.getDeviceCount() < 1) {
    throw std::runtime_error("No audio devices found!\n");
  }

  dac_.showWarnings(false);
  params_.deviceId = dac_.getDefaultOutputDevice();
  params_.nChannels = 1;
  params_.firstChannel = 0;
}

template <unsigned int frames>
Audio<frames>::~Audio() {
  if(dac_.isStreamRunning())
    stop();
}

template <unsigned int frames>
void Audio<frames>::play(PacketDeque ptr) {
  buffer_ = ptr;
  dac_.openStream(&params_, NULL, RTAUDIO_SINT16, sampleRate_, &bufferFrames_, &Audio::transfer, this);
  dac_.startStream();
}

template <unsigned int frames>
void Audio<frames>::stop() {
  dac_.stopStream();
  if(dac_.isStreamOpen()) 
    dac_.closeStream();
}

template <unsigned int frames>
int Audio<frames>::transfer(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData) {
  Audio* audio = reinterpret_cast<Audio*>(userData);
  int16_t* outBuf = reinterpret_cast<int16_t*>(outputBuffer);


  typename fast_deque<AudioPacket, DEQUE_SIZE>::iterator it;
  bool valid = false;
  {
    auto resource = audio->buffer_.value()->lock();
    it = resource->pop_back();
    valid = resource->valid(it);
  }
  
  if(valid)
    std::memcpy(outBuf, it->data, nBufferFrames * sizeof(uint16_t));
  else
    std::memset(outBuf, 0, nBufferFrames * sizeof(uint16_t));

  return 0;
}

#endif