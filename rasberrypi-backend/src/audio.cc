#include "audio.hpp"

#include <iostream>

Audio::Audio(unsigned int sampleRate, unsigned int bufferFrames):
  dac_(), params_(), sampleRate_(sampleRate), bufferFrames_(bufferFrames) {
  
  if(dac_.getDeviceCount() < 1) {
    throw std::runtime_error("No audio devices found!\n");
  }

  dac_.showWarnings(false);
  params_.deviceId = dac_.getDefaultOutputDevice();
  params_.nChannels = 1;
  params_.firstChannel = 0;
}

Audio::~Audio() {
  if(dac_.isStreamRunning())
    stop();
}

void Audio::play(uint16_t* buffer) {
  buffer_ = buffer;
  dac_.openStream(&params_, NULL, RTAUDIO_SINT16, sampleRate_, &bufferFrames_, &Audio::transfer, this);
  dac_.startStream();
}

void Audio::stop() {
  dac_.stopStream();
  if(dac_.isStreamOpen()) 
    dac_.closeStream();
}

int Audio::transfer(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData) {
  Audio* audio = reinterpret_cast<Audio*>(userData);
  int16_t* outBuf = reinterpret_cast<int16_t*>(outputBuffer);

  for(unsigned int i = 0; i < nBufferFrames; i++) {
    outBuf[i] = audio->buffer_[i] - 32768;
  }

  return 0;
}