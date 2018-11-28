#include "numtypes.hh"

#pragma once

constexpr struct Frame {
  s1_15 l = 0._s1_15;
  s1_15 r = 0._s1_15;
} zero;

constexpr int kBlockSize = 16;
constexpr int kSampleRate = 96000;
constexpr int kNumOsc = 16;

struct Parameters {
  f pitch;                       // midi note
  f spread;                      // semitones
  f warp;                       // 0..1
  f twist;                      // 0..1
};
