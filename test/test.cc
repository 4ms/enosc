#include <cstdio>
#include "parameters.hh"
#include "dsp.hh"
#include "data.hh"
#include "wav_files.hh"
#include "polyptic_oscillator.hh"

constexpr int kDuration = 10;    // seconds

struct Main {
  WavWriter<Frame, 2> wav_{"test.wav", kDuration * kSampleRate};
  Main() {
    int size = kDuration * kSampleRate;

    Parameters params;
    PolypticOscillator osc {[](bool){}};

    while(size -= kBlockSize) {

      Frame out[kBlockSize];
      Block<Frame> output {out, kBlockSize};

      osc.Process(params, output);

      // write
      wav_.Write(out, kBlockSize);
    }
  }
} _;

int main() {}
