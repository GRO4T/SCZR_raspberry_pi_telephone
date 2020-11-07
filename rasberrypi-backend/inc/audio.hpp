#ifndef __AUDIO_HPP__
#define __AUDIO_HPP__

#include "RtAudio.h"

class Audio {
  RtAudio dac_;
  RtAudio::StreamParameters params_;
  unsigned int sampleRate_;
  unsigned int bufferFrames_;
  uint16_t* buffer_;
  double test[1];
public:
  Audio(unsigned int sampleRate, unsigned int bufferFrames = 256);
  ~Audio();
  void play(uint16_t* buffer);
  void stop();

  unsigned int getSampleRate() const { return sampleRate_; }
  unsigned int getBufferFrames() const { return bufferFrames_; }

  static int transfer(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData);
};

#endif