#pragma once

#include "dsp.hh"

namespace Distortion {

  template<WarpMode> inline f warp(s1_15, f);
  template<TwistMode> inline u0_32 twist(u0_32, f);

  // TODO rewrite with Numtypes
  template<>
  inline u0_32 twist<PULSAR>(u0_32 phase, f amount) {
    u0_16 p = u0_16::inclusive(amount);
    uint32_t x = (phase.repr() / (p.repr()+1));
    if (x > UINT16_MAX) x = UINT16_MAX;
    x <<= 16;
    return u0_32::of_repr(x);
  }

  // TODO rewrite and optimize
  template<>
  inline u0_32 twist<DECIMATE>(u0_32 phase, f amount) {
    uint32_t x = phase.repr();
    x = (x ^ (uint32_t)(x*amount.repr())) * Math::sgn(x);
    x ^= (uint32_t)(UINT32_MAX/4 * amount.repr());
    return u0_32::of_repr(x);
  }

  template<>
  inline u0_32 twist<FEEDBACK>(u0_32 phase, f amount) {
    return phase;
  }

  // TODO optimize
  template<>
  inline f warp<CRUSH>(s1_15 sample, f amount) {
    f x = sample.to_float();
    union { float a; uint32_t b; } t = {x.repr()};
    t.b ^= (uint32_t)((t.b & ((1 << 23)-1)) * amount.repr());
    // t.b ^= (int32_t)(((1<<23)-1) * amount.repr());
    return f(t.a);
  }

  // TODO optimize
  // TODO amount: [0..1[
  template<>
  inline f warp<CHEBY>(s1_15 x, f amount) {
    // TODO comprendre -2
    amount *= (Data::cheby.size() - 2_u32).to_float();
    index idx = index(amount);
    f frac = amount.fractional();
    u0_16 phase = x.to_unsigned_scale();
    f s1 = Data::cheby[idx].interpolate(phase);
    f s2 = Data::cheby[idx+1_u32].interpolate(phase);
    return Math::crossfade(s1, s2, frac);
  }

  template<>
  inline f warp<FOLD>(s1_15 x, f amount) {
    constexpr Buffer<f, 12> fold = {{-1_f, 1_f, -1_f, 1_f, -1_f, 1_f,
                                    -1_f, 1_f, -1_f, 1_f, -1_f, 1_f, -1_f}};
    f sample = x.to_float() * 0.5_f + 0.5_f;
    amount = (amount + 1_f/fold.size().to_float());
    f phase = sample * amount;
    return fold.interpolate(phase);
  }
};
