#pragma once

#include "dsp.hh"

constexpr int sine_size = 1025;

struct DynamicData {
  static s1_15 sine_table[sine_size];
  static constexpr Buffer<s1_15, sine_size> sine {sine_table};
  static s1_15 sine_table_diff[sine_size];
  static constexpr Buffer<s1_15, sine_size> sine_diff {sine_table_diff};

  DynamicData() {
    MagicSine magic(1.001_f / f(sine_size));
    for (auto& i : sine_table) {
      i = s1_15::inclusive(magic.Process());
    }

    for (int i=0; i<sine_size-1; i++) {
      sine_table_diff[i] = sine_table[i+1] - sine_table[i];
    }
  }
};

s1_15 DynamicData::sine_table[sine_size];
s1_15 DynamicData::sine_table_diff[sine_size];
