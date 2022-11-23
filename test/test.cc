#include <cstdio>
#include "parameters.hh"
#include "dsp.hh"
#include "data.hh"
#include "wav_files.hh"
#include "polyptic_oscillator.hh"

constexpr int kDuration = 10;    // seconds

struct Main : Math, DynamicData {
  WavWriter<std::pair<s1_15, s1_15>> wav_{"test.wav", kDuration * kSampleRate, 2, kSampleRate};
  Main() {
    int size = kDuration * kSampleRate;

    Parameters params = {
      .balance = 1_f,
      .root = 30_f,
      .pitch = 30_f,
      .spread = 0_f,
      .detune = 0_f,
      .modulation = {.mode = TWO, .value = 0_f},
      .scale = {.mode = TWELVE, .value = 0},
      .twist = {.mode = FEEDBACK, .value = 0_f},
      .warp = {.mode = CHEBY, .value = 0_f},
      .alt = {
              .numOsc = 1,
              .stereo_mode = ALTERNATE,
              .freeze_mode = LOW_HIGH,
              .crossfade_factor = 0.5_f,
      },
      .new_note = 42_f,
      .fine_tune = 0.5_f,
    };
    
    PolypticOscillator<kBlockSize> osc{params};

    f root = 20_f;
    
    while (size -= kBlockSize) {

      f noise = (Random::Float01() * 2_f - 1_f);
      noise *= 0.1_f;
      root += 0.001_f;
      params.root = root;// + noise;

      Buffer<Frame, kBlockSize> buf;
      osc.Process(buf);
      // std::cout << buf[0].l.repr() << ", "
      //           << buf[0].r.repr()
      //           << '\n';

      Buffer<std::pair<s1_15, s1_15>, kBlockSize> output;
      for (int i = 0; i < kBlockSize; i++) {
        output[i] = {s1_15::wrap(buf[i].l), s1_15::wrap(buf[i].r) };
      }

      // write
      wav_.Write(output);
      // std::cout << size << '\n';
    }
  }
} _;

int main() {}
