#pragma once

#include "dsp.hh"

static constexpr int sine_size = 512 + 1;
static constexpr int cheby_tables = 20;
static constexpr int cheby_size = 512 + 1;

struct DynamicData {
  DynamicData();
  static Buffer<std::pair<s1_15, s1_15>, sine_size> sine;
  static Buffer<Buffer<f, cheby_size>, cheby_tables> cheby;
};
