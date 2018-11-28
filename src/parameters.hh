#include "numtypes.hh"

#pragma once

constexpr struct Frame {
  int16_t l = 0;
  int16_t r = 0;
} zero;

constexpr int kBlockSize = 16;
constexpr int kSampleRate = 96000;
