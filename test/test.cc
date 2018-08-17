#include <cstdio>
#include "parameters.hh"
#include "dsp.hh"
#include "data.hh"
#include "wav_files.hh"

constexpr int kDuration = 10;    // seconds

struct Main {
  WavWriter<short, 1> wav_{"test.wav", kDuration * kSampleRate};
  Main() {
    int size = kDuration * kSampleRate;


    printf("%f\n", (0.25_u0_16 * 0.5_u0_16).to_float().repr());

    while(size -= kBlockSize) {
      // process by engine
      // conversion to short
      short output[kBlockSize];
      for(int i=0; i<kBlockSize; i++) {
        output[i] = 0;
      }
      // write
      wav_.Write(output, kBlockSize);
    }
  }
} _;

int main() {}
