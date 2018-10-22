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

    FourPoleLadderLp lp;
    MagicSine sine (0.000002_f);

    while(size -= kBlockSize) {
      // process by engine
      // conversion to short

      Float out[kBlockSize];

      for(int i=0; i<kBlockSize; i++) {
        Float rnd  = Random::Float01() * 2_f - 1_f;
        // if (size < (kDuration-1)*kSampleRate) rnd = 0_f;

        Float cutoff;
        sine.Process(&cutoff);
        cutoff = (cutoff + 1_f) * 0.1_f + 0.01_f;

        lp.Process(rnd, &out[i], cutoff, 4.0_f);
      }

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
