#pragma once
#include "numtypes.hh"

struct Signal {

  static constexpr f crossfade(f x, f y, f phase) {
    return x + (y - x) * phase;
  }

  // (p..1 -> 0..1)
  static f crop_down(f p, f x) {
    return ((x-p) / (1_f-p)).max(0_f);
  }

  // 0..(1-p) -> 0..1
  static f crop_up(f p, f x) {
    return (x / (1_f - p)).min(1_f);
  }

  // p..(1-p) -> 0..1
  static f crop(f p, f x) {
    return ((x - p) / (1_f - 2_f * p)).min(1_f).max(0_f);
  }

  template <typename T>
  static constexpr int sgn(T val) {
    return (T(0) < val) - (val < T(0));
  }
};
