#include "numtypes.hh"

#pragma once

constexpr struct Frame {
  int32_t l = 0;
  int32_t r = 0;
} zero;

constexpr int kBlockSize = 32;
constexpr int kSampleRate = 96000;
