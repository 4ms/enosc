#include <cstdint>

#pragma once

struct ShortFrame {
  int16_t l = 0;
  int16_t r = 0;
} zero;

enum I2S_Freq {
  I2S_FREQ_8000 = 0,
  I2S_FREQ_11025 = 1,
  I2S_FREQ_16000 = 2,
  I2S_FREQ_22050 = 3,
  I2S_FREQ_32000 = 4,
  I2S_FREQ_44100 = 5,
  I2S_FREQ_48000 = 6,
  I2S_FREQ_96000 = 7,
};

constexpr int kBlockSize = 32;
constexpr int kSampleRate = 48000;
