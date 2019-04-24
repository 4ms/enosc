#pragma once

#include "dsp.hh"

constexpr int sine_size = 1025;

struct DynamicData {
  static Buffer<s1_15, sine_size> sine;
  static Buffer<s1_15, sine_size> sine_diff;

  DynamicData() {
    // TODO 1.001 -> size_size + 1
    MagicSine magic(1.001_f / f(sine_size));
    for (auto& i : sine) {
      i = s1_15::inclusive(magic.Process());
    }

    for (int i=0; i<sine_size-1; i++) {
      sine_diff[i] = sine[i+1] - sine[i];
    }
  }
};

Buffer<s1_15, sine_size> DynamicData::sine;
Buffer<s1_15, sine_size> DynamicData::sine_diff;
