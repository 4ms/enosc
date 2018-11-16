#include <cstdint>

#pragma once

constexpr struct ShortFrame {
  int16_t l = 0;
  int16_t r = 0;
} zero_s16;

constexpr struct LongFrame {
  int32_t l = 0;
  int32_t r = 0;
} zero_s32;

constexpr int kBlockSize = 32;
constexpr int kSampleRate = 96000;
