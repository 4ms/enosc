#pragma once

#include "dsp.hh"

constexpr int sine_size = 1025;

static constexpr int cheby_tables = 12;
static constexpr int cheby_size = 512 + 1;

struct DynamicData {
  static Buffer<s1_15, sine_size> sine;
  static Buffer<s1_15, sine_size> sine_diff;
  static Buffer<Buffer<f, cheby_size>, cheby_tables> cheby;

  DynamicData() {
    // TODO 1.001 -> size_size + 1
    MagicSine magic(1.001_f / f(sine_size));
    for (auto& i : sine)
      i = s1_15::inclusive(magic.Process());

    for (int i=0; i<sine_size-1; i++)
      sine_diff[i] = sine[i+1] - sine[i];

    // cheby[1] = [-1..1]
    for (int i=0; i<cheby_size; i++)
      cheby[0][i] = f(i * 2)/f(cheby_size-1) - 1_f;

    // cheby[2] = 2 * cheby[1] * cheby[1] - 1
    for (int i=0; i<cheby_size; i++)
      cheby[1][i] = 2_f * cheby[0][i] * cheby[0][i] - 1_f;

    // cheby[n] = 2 * cheby[1] * cheby[n-1] - cheby[n-2]
    for (int n=2; n<cheby_tables; n++)
      for (int i=0; i<cheby_size; i++)
        cheby[n][i] = 2_f * cheby[0][i] * cheby[n-1][i] - cheby[n-2][i];

  }
};

Buffer<s1_15, sine_size> DynamicData::sine;
Buffer<s1_15, sine_size> DynamicData::sine_diff;
Buffer<Buffer<f, cheby_size>, cheby_tables> DynamicData::cheby;
