#include <cstdio>
#include "parameters.hh"
#include "dsp.hh"
#include "data.hh"
#include "wav_files.hh"
#include "polyptic_oscillator.hh"

constexpr int kDuration = 10;    // seconds

struct Main {
  WavWriter<short, 1> wav_{"test.wav", kDuration * kSampleRate};
  Main() {
    int size = kDuration * kSampleRate;

    PolypticOscillator osc;

    while(size -= kBlockSize) {

      Float out[kBlockSize];

      osc.Process(out, kBlockSize);

      short output[kBlockSize];
      for(int i=0; i<kBlockSize; i++) {
        output[i] = s1_15::inclusive(out[i]).repr();
      }
      // write
      wav_.Write(output, kBlockSize);
    }
  }
} _;

int main() {}
