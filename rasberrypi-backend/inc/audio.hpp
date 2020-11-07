#ifndef __AUDIO_HPP__
#define __AUDIO_HPP__

#include <optional>

#include "RtAudio.h"
#include "ipc.hpp"

template <unsigned int frames = 64u>
class Audio {
public:
  struct AudioPacket {
    uint16_t data[frames];
  };
private:
  RtAudio dac_;
  RtAudio::StreamParameters params_;
  unsigned int sampleRate_;
  unsigned int bufferFrames_;
  std::optional<shared_mem_ptr<fast_deque<AudioPacket, 1024>>> buffer_;
public:
  Audio(unsigned int sampleRate);
  ~Audio();
  void play(shared_mem_ptr<fast_deque<AudioPacket, 1024>> ptr);
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
void Audio<frames>::play(shared_mem_ptr<fast_deque<AudioPacket, 1024>> ptr) {
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

  auto it = (*audio->buffer_.value()).pop_back();
  if((*audio->buffer_.value()).valid(it)) {
    for(int i = 0; i < nBufferFrames; ++i) {
      outBuf[i] = it->data[i];
    }
  }
  else {
    for(int i = 0; i < nBufferFrames; ++i) {
      outBuf[i] = 0;
    }
  }

  return 0;
}

#endif