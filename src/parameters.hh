#include <cstdint>

#pragma once

constexpr struct ShortFrame {
  int16_t l = 0;
  int16_t r = 0;
} zero;

constexpr int kBlockSize = 32;
constexpr int kSampleRate = 96000;
